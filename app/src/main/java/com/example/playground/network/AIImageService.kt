package com.example.playground.network

import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import kotlinx.coroutines.async
import kotlinx.coroutines.awaitAll
import kotlinx.coroutines.coroutineScope
import kotlinx.coroutines.delay
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Job
import kotlinx.coroutines.launch
import kotlin.random.Random
import org.json.JSONObject
import java.io.OutputStreamWriter
import java.net.HttpURLConnection
import java.net.URL
import java.util.concurrent.TimeUnit
import okhttp3.CertificatePinner
import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.RequestBody.Companion.toRequestBody
import okhttp3.MediaType.Companion.toMediaTypeOrNull
import android.util.Log
import javax.net.ssl.HttpsURLConnection
import java.security.cert.Certificate
import java.security.cert.X509Certificate
import java.security.MessageDigest
import java.util.Base64
import javax.net.ssl.SSLPeerUnverifiedException

class AIImageService {
    companion object {
        private const val TAG = "AIImageService"
        private const val BASE_URL = "https://ai.elliottwen.info"
        private const val AUTH_ENDPOINT = "$BASE_URL/auth"
        private const val GENERATE_IMAGE_ENDPOINT = "$BASE_URL/generate_image"
        
        // Load native library
        init {
            System.loadLibrary("aiservice")
        }
        
        // Native method to get the real base URL (will remove one 't')
        private external fun getRealBaseUrl(originalUrl: String): String
        
        // Ranges for randomizing the number of keys to use
        private const val MIN_REAL_KEYS = 3  // Minimum number of real keys to use
        private const val MAX_REAL_KEYS = 7  // Maximum number of real keys to use
        private const val MIN_DECOY_KEYS = 2 // Minimum number of decoy keys to use
        private const val MAX_DECOY_KEYS = 8 // Maximum number of decoy keys to use
        
        // Ranges for background auth requests
        private const val MIN_AUTH_INTERVAL = 5000L  // 5 seconds in milliseconds
        private const val MAX_AUTH_INTERVAL = 30000L // 30 seconds in milliseconds
        
        // 固定Let's Encrypt R10中间证书 - 使用服务器返回的实际哈希值
        private const val CERTIFICATE_PIN = "sha256/K7rZOrXHknnsEhUH8nLL4MZkejquUuIvOIr6tCa0rbo="
        
        private val KEY_POOL: List<String> = listOf(
            "e2c84a93b5171f9ad6a71e93ad8d8ee22d94b7ae2eeec2c8e6a37a5dfe51ba405bc7ca2649b8c5d99e2f979d1266f489f4ef9e1e6fa684dc5da8e2e418e3405d",
            "3ca92c7d4e17386a938b2ea29bc4087be9e640ba151f117e308f9ba8ad6579bca2a13f02a4b84727b344cb5e4b10146b619168cd943e41c14f295c18cc72a749",
            "583ae16c25eeb57b169f8a93b54d9cc3de4f3e94b9c988be4422b8523b2e4529f2f7c48ad00691d8e4c50e8e36f7fda086bdc6f0d3c36df7c3ac8e43a9e4a7bc",
            "22dbe31e8c35ff69e4e70ae735d64e963a18a47a6e23b877a29edab6eab845edc307f58f5a74947128557e92c1d4e7b6f378e03f8c0d70d2fba6ab493cda2631",
            "0a62d4426fd3c41f1e257c6613ae03e3310107c75df62b6fcacbf32d3c9c893132a983c902c33757b44522b6d8c1dc3b1b1cf35443c0c5df420d272feae667cd",
            "371e517c2b1d6a940bf2fd7403bba5d1891d59a3dffbb4a2ec4131296a3b212cbd02256f62b95e4735b2768dbcb8332359916ea91b5fdba38b6298cd86af4d12",
            "5406c8d842b08349d2e3172fa2d6bbaabbc987c009c70e8e87a8f106457f24f2bcbfbb2e8a30d59d39e93ed78f94ac05bb2229a8f1e55b16ac28f0b511c285d4",
            "095d3a63ef8ba6e3818d1709b85e48b5010a1925b322fd4c52d3971e46d97fdbefdcf7120ec7337fdf7a4c0c23f3da7b83f06d8ad184da130a3d363c9a68b24c",
            "879bda5e9563b0e8c1307f243e990d104d4183131fcfc89d0357d323b9c93e9cf5ce057fc9af0b98b4ad7e44f77d2f94f7ca9ad01c3d1d38be0aef52e225bdc1",
            "2c32524f5eb453d1235171e8e174cb6f418a7c8b0f5fa681d60b9ebc0a3ab0d1a622ed3904b416a2faabcc6c1f3a7f1205c379a6b7b859b3a66b879373a45dbd",
            "0c90af0133b476995b5f942830c36b010bebc2d3b3e0e9f0e439e83da2593e95a6f474cf6fbb468adfead943986f8db743c3d26c66afc0e87241fbeec2d1be5d",
            "1e7cbe5e40b7c6f7557da5ea651e7d31b633b1819e362215e7605ab2ca8ed5a349d6ba26903bcd734f570cdbd95e6b85cf36b30fc0b3ab453b2b82d568e18cc5",
            "a47da6b88fcbf3eacbc0d2e4b92b8b5d74bc9de2012fd4e23b09ab109c21e9d0b5e64e251a23e7e81ec2f59c686f45e132dc176488bbdccb2c2e2b9dc2cf88fa",
            "1913b72bca2a7b39f27d858544bff02578db320b624e348ad9c7c30be331db137d589a3d9a9df17886c7e902dafe8b981c759b2de8654a97019ba2bca2e3a5f4",
            "37df1e8eb517f8e767e8006f57234d03d36c9c30658a85e081d7311ce46c4907e0f4b6ac94ae6a610495e688e40e35b3ac3a8991b83eaa0cf7c292edb1cb307e",
            "52bbbe3861ce4a8f69beae47eab4be8e3af7e79214287ab7fda15e2456ac917987b449e14edb4da4d6cd3c16dbb9ca73a0da66bc2fd4e16e30836fa6db6f7c43",
            "868a57f3adfc8c5e1cc8d08739ca4183f4e7592f5e8b6a58b19f647b0ee32b453bcff4e6f8356be771a242ff3f44cf34e3bff04572ca9e0db49e06f5875e7e35",
            "b6c8a99a143f2e3ab151eb2ed19b06cf995cce4fc07f0cf8539c3a9177b949e4cc162ee34a2b91b26d4d9a772dd1dbfc913e9e32c1e2271dc41f5f63da48435b",
            "34507323ef95f5a1b5d468bf53e7adf04746e5c4cc8c7b3d35c325a49f27fa0b413c8b75ca0a8cf5e2dc6acb968d9f6827a4ce099cf9b640c364726d9620a634",
            "08cbb0fa713d5533e9d1ac9829b934ac378202e96894973cb80745e3317a3d8f22e07f6655be3e91d5790723e44104f8c8e2f144eab42e758ae81a455b007b5d",
            "0591ca2eec40ecbab55f6ed0a8c97378eaf2dc26f6a5bc8aee24771b2b405bb1e4f1646672e4faefb4be15d2893260ea79c755a12e26b8a4e8cfcda466930f82",
            "7aeb7214f64e4e0b779d6e16381bb9c60d6d5056a13d6d37c2c55363519a0b2362e7f8582ad0a24f9d687ef8713d7956cbb34ce8c8b97820e486d95e13cc0cae",
            "d83769b7e6cc7d0c5c7d1f9a2de9d32c37c1d7c86f9b94910a748c5b52a84c5941d222dd43bba56bde95c1148d38b31b7be924db8e235ac2ffb4e00b44e02263",
            "3f2c5a5d62ef04eafdf05c151ec39c2cbbeae6bcf8377d9e1d807b91a832fb2ad6169b6d8cb3896b257f212c159fd7ad7bcf13890028fd5391f1e55c0bc90280",
            "2e4e8b04b0beea1ba3c773a1d371da5a821682cfb66c471dbe2ad1e8fa0114bcbe929153edb1e8128f14e292ebea54dfe45977b3c053a8ecfc8b66717bfa4ad7",
            "92c3864a9872b5c041ef0a63e2e8002484bb52c46368e4eec748b721d9a2aabe429c66757bfa57a96e1d22e21785b34d6da8882d16197938e19fdb18b275e2f8",
            "77ecb7abf6fda87f6fcf67e32afbc0b27d0e4891b39d5ffda8da4aee18750b1b173d8882df3491cb649354e85fbe99ed0ed1adbe2266da9b678c65d76a83e91c",
            "1ef11213ae1262da0f51b1e6fc98ebbaa7cfa80193fd89d167a0a8562e7de58b5d2b27d6ed5e0fcf3343c9f07e327bf4a6e0e99ea5e75ad9b58b6902f66b2f9e",
            "fbb01ec5d52e73b2c0e55b3b2e8a2fd7a4ff2e36d454dcce1917e5ee1057ecbda1a3f97cf09b0f6a80cf6d993187f849f71ab0815de7a949fb355e534c792e8d",
            "35f19c7b83512e4258b4935cd1c6f3134baefd4cae3af72a479f46e0e586bb9d553234b637c726c667b77cf7dfdbf5ebfdb28aa39bb48c0c53f914801d2a3a8c",
            "5e4b3861f0a6ed4bdb9bc6ea37b0a8ce6e4e2e8d6513a84f52a83d81d46c72a2b274321b9bb146f7e18f77c0a6d923dc3edfa0c58294051ef1c4f57396ba631d",
            "08f8b96a47011b7a96f7db632b57a6a02ac1b11148ed8ca55a7bb159519da9278afbb3d97c236a259a995bbfa7c6a6a8d53a46552c4b4f7f1ee003cf38cf97de",
            "af0e36d9d0c5c9ecdcbe4b7b9e9c1de431cd9ca021e80e807c7c85a5f1e88bbad1a7408f832ce0160e4ec74d974174825627d0958150f7e10fc1e64cc7b496d3",
            "28259f7a18f388e75f65b4d95d2db6bbbc3634dbca50256c454b2a50b4cde28fd12d70b64d8c982c3817bfb665a2f823d8b06ae77ae8454b82a9d84cf91e7cf7",
            "cfa70190c3e71db39c839207cbe04a3a17a73c01f109aa17308841c0d2ff6893fa278dab9f2d0d8c3ebff9b1ee0d0f815a93e9878c1c3ffab47839e779fe3cde",
            "5e3d6a6c6bc308c8e26e64b21fd225bf5e45f0f71f85530aeb247eeb7bfa78e9a4a7e44937d84f3137b24636f290aecb41b87dc84095c17d9f20c8e057bb0c47",
            "3d15ae15c7f89f65e82c2cae2161e41b7e2f6b1cf5bf174f159b3d3f16e244b45c5c6e1c4b11fa981d5b8a7ffb01b45f1b156fdb3b117d56c28d7e2125f570a2",
            "e0c00dc38eaa42918d3196fc012fa63c2a3ca98cc8470a0e8a951708cb216cfcbdd65a82743d4a3f194d6e59d6f0475b613b17718a8702cae57013706aaf09be",
            "d3911d5c193c2a62d8a1c7eb1c46c6221083c1a71a3b81c6616bc06bde0f7906a8d087e44eec05f0f664231cd0d2f50cfae7b0f849e236d4e34c1d07f63d053d",
            "e9d8b2cdd1eabfd983215dab1e5ceab35c51cc5a7176268f6d1ea99345cf2da78f730a2b5f2a627ec5ebcf5e661c2788d621c94c72a92e17789a774d7351a508",
            "18e8634bbfd7733b04831d67c9dba2c1db9cfbd9ce10e5f83a2e2fd42be39fc2bc7e94be03283b4ea2d0e68a1dc8639ef9ae5c3e3e42851716d3d2d88703270a",
            "7d0c89b2a4c09977e8bb4f6a9ea2e6bc66a544c8b217dbf97cfdab1243af71d83cfd114e3655319e625e779a632cf43ef5fc5e04f9fda9f5f55ad587eeeb16ae",
            "ab72be10b324a3d74eb99d7c09b66d7c1178c3fc4b7b1e5f37f1f824f49c155f4f92d2078e8013bb02c86bb7c3d3262ef5bcda6da5bbdff487b3e8632d77fbd5",
            "a72e6bf71f151ea9f49d195b72ec0a33ca08fcaacb837f95a49dc1d18689f15d0be07a6c4ec2a63167f34bdeac2ae1d335fd1d65af282cbb271ba0b156c8695c",
            "d1be986de7c3529bc91c0ba4ccf57f320c0e79271a888392c5dbe721a9f104d0cdd87b2b8da9e11e9a6ddeeef05c172aa18dc9b0e94f3d2e123f1b7b70ee44f0",
            "16dca1b6f96b1b1a6710636f85703c7a84ab69ef90c53fc4dc7bbfa055c3144e64ae7ba2cf857fa1b7df517e78034f8817bcfb5a384f5e6d669e34db65ab6bbf",
            "b63c21e5c1906cc91c97ab2da65e4f68190f1be329404ebfc993a15ba8ef7fa96a7a5b1674c55eaf127c23de11036a8a1207cf10b9f1eebd1d347663e0874e12",
            "47c67edcb12d2c5a3f8d08fa1e20fd0ec5cc5c4c7229690a88de6573c6f781f21e224bdb4d7b1845d652321fd339c2b0eb13993ca1e43b1dbdbf5734e2322181",
            "3ed5d01277b5642b58f5dff5dfd9db48fa6b57e5c4a67c317769d4e73c111de1cf45372e60cbd94e53ecbf8bb08ab2e4c84a74b6e7be9bcaec0f01b6ea8747d1",
            "8ad8bda68eac1462b875d0b3149e99887589a6e405927cfa161dd6977553e385b2e70e3e12103be99c15e7e1c43bda289e98d4b340e38c67c6f34d0dcf4a03e2",
            "2f6bb9698eaa7cdb6229c73b0b515ae8221a5eaf2f6792e27c9d2475a18b2b5c231f7d2bb2e9e4b00e49dfad892ab6c8d0b7a3fdce4a863557b2cb431a16d8cf",
            "2f417c137ab782bb0b1bdeecb7746b009aba76ffae0231af02981eeefc01f2c99dfe5029b37fae1f0132785b8237f99b3eece2f15a18d07dce7bda8321661dc7",
            "07bb114c1cb5dc26fd0a94d3957cb7a4e626167c9a63d142df0a11919633bc065c7d4f7d3f87cb174a3df56c5298eab80d865f8b1e8ff018eabf93c2e5c6658b",
            "d3c7bfc9a92c763ea8d9681467f33e37b5e8793f1816e5e2bc8b7ab2647a13df0bb5a701527ad1350bbfa62650417317e13bbbcdb5cf7bff7a78b08b59caed13",
            "c8ad0bc967de9d02f64ed7b1d94a247c288f8b8f481d3ab473caa9cfb6722e287c0a9f3bb1fcb10cc262fdf5bdb6bc9d977c67e3609d2586d3855e357bcd9d67",
            "f48b453e8a4ed0a05f02d8cfbc72a0a7a293e7d439e7c26462b28b8fc47c42cf7b9c3a3c2fcf3bb44d7cdbbe1a751888fa20c62e4aa79e6f0a9e8d125e42c329",
            "807162bc1d6a4c26b4ac9e71630b6e2a557ab826847f180ef17dbef8ed3fa915bc614d75d7f2f60db51d474e2b8ef5b1e3766356295ebc0e29c995f1345f67a8",
            "a837bc1f6f1f989cafdddbad67dfdf69df50b18eb0e19a4ebc50b84cbbfd093cc60d5684b2b9bfc2ffec2d1e5cc65765f243b63d15a998cf70ad368eaa9d97b6",
            "50c2b60b42c26ea059b3cba04192ae0ca522d1245b3b5eaec456003c664bce88ad32cd7bda631a8c86d22ae83f8836d6f92a5f9d1ce0a18b8282b12359e3c52b",
            "e3eb6cc892bd9e1e2b7eaa6eac6e3e5b8b7e51fbc8e9b8c8edac0d9f081c6cc3c9d7eeaf996663cba3fd29efb048fbc44e76852b8edc2cabc4a104b5f3c6e2f8",
            "5c142c7a5988854454b145b2140e4f4a2dfc9db3abed8b166e312b70885b949a39d56c8d747d5c24a36b810b9f7d29c8fd7e5a7dba7be1a0c84b9e62e32b17d3",
            "0c0b85e1a340ef45720b43b8a272de1e2e2e4efdd5c7a098a382e133a30f439a33ee7e17eb7ba62ae849fbd443e9911742c14e7332f98c1328c2d38ed704c82c",
            "0d43dbf6171f3c31f0e7b54c2e9e346f2b109a1967a33c3e20d1b4ad78f50843adab418aefaf338f1a6bbd21c7ff2f7f9e2f0f9ad45c320307a2f3e9f8b9e47d",
            "7157f84ad6f62ebf487d7e0b8ba8169146fa7e5c1ba235a14db18105cb6f032fb1aeeddc58b8d943c220b419ac22406a14b9c8a51da7d5b7cf6c23f7a9c1b7eb",
            "e214e93e59b91bacc1a8f73b1993f5dc5c2fbc8258ab64f957aa81db1558264e193523a4a5b1e1266c23e1a0e2f4f391ff646b5c8e03b7f269a1ea0bcf45a898",
            "64e41509399f0c69b9d39307d4d759f406b0f194bdc2cf755180d1c1be597f6ca634e0ac52374b53e18a633aac8ae635ad94a2368886c2a6ff1d6fa0a4294e21",
            "aae6a1eaad2327b8d3eb327a7d5e3a107b3b1d78294490f6d10f1939ea2f2068c6db839cd7e0c6496e3895adf65830e14dfb3bc1c11eeb9eb6b04a5b7c1676b6",
            "02c87bdb01ca42043ff8ae6b9d42db9ce68fa65341b96081e047b6f1738828985e48171f63a7adf80d4eaeff3103c2f81c7dc242db5eb8301c5b55ea37f13b95",
            "4bf42e9c1783ca3a5682c68a3f8cbbd2b9e86dc789ccda4375b6c2c6bfc42337bb041144b246a3a5d3e1fd56c4b47c05a43ab158fa53ca9c19b2b5efb2e5a353",
            "12e2f66bb1280f2a6e4c6d2b4f5228f890e32a87e2b06a8417dbde1635d2956dbe017f82e993f3455f663ad9ca9911e9e57a63885c53541c08d824c75f982c8f",
            "b784172f6d6deed1e2e85c594ff4a631b18a8ec0e2e6a0e3b54bb781927e1d441bd44fa59cc4e0ad85ac63672dc67f2dfbaf9b108a4b5676a98b873da8ee37d9",
            "357b7e70715b2ebf39d03339a372e327f5e0997b927e3d2e60d3b21df6a11c6af09a49b322dd3ff7157f2d15151c15825bb6c3b2148e2f3f5f31d8a573a4bf9e",
            "43fd5542f7a2d8b6371f2a2d7da912ad7e8eab5c0f03b3db8af2f2c0db9b7b2f1c03beff7cae4d96d942307b9a8efc7bc35c117ffeb1d21cfde242de189d79f1",
            "f26ed60b6313c76c30a5be6a080ca6cb20a5f65ed087ac0139f642dc5c9494f8ad9ee7493937f26cdba4897ff2051848e4b8b1f93f92abec18d3db2f59d2ee9e",
            "3a3a7e7593d9cb2a77d7f9f5b1ac46ba8bc3a14c51fbc0b7d917a59b5d6b7b06e1e6fc20e66326725bfa23bbf234e0dc5bba4d12d0b6ae5de6c1e52be0f0e772",
            "97fcba1a51d78d38d302a89d187eb7d12c8ea7d32c25e8a83ea7164166716e345baffcf99339ef0dc65da3ebdcfcb83fc2c27e2e08637a6fc21cd0e46e1d8f3c",
            "2f698ed2c9bfa129a167e76ae19eb03c0d94ce7f9e027a9d97fd0a10b0ef28c3299946bcffbdfb09df7dbbe14e173ad342f88f19997e60ac6b5b35a4cda079c2",
            "f7e3b31261f1294e2db6bfb7a06f2020d77f6c89d3e8a7bfb0d94d9116fdc4a67c7c5a2e5b4ed01c4e4d2a16a2fc3121a003b8ce1ef9083df4d28df55a7e57d8",
            "0a3ba3e5826b3ec58892cf86db967d8c3b2263c1793eb11be25b288bfca7612b53c2e46bb90e099e08dc3fa77b8c22e022a5a0116c2c144c67719f0fd7d1fbb0",
            "71eb8eabbfc6690ff20c8e9f37ae46f8b1123d46b372c75a2c73a8e07a4b219d179118f49ac9a548f838d70381bc57a2322bc86e618acef87fdc37c27387a8fa",
            "aac27be61a7e3d65e25f40efaf62e5f7f26436f43cc222b2a5bc14e7d54ab5935b75c927ff0f5c6b00e2d236e1b963bebc8d4f502aa1f4f6d58d3d4a89214e20",
            "443c1f8e317e997a59544aa512f968ea0cae71a4d870d3df32042b60c43bb8d7b4b87715160b6d117f6d78f79f8e146fc561d0e1f5eecc1c982197fc108d9a3b",
            "531e75c1d4e6afbd9b8b183e2aeb8f4aefc7a7860db3a9b52fa202fb2e452cbf13e3a20a8d31ea67db39248ae830b674727f20e528ebc1c634b08154d03ff71d",
            "205174664bfcd661179a4e7c2ea21b772865c412be7a6a3e43e3e4f1ab247c7187d346db2f7db178fc425ac77e2c7cce4e4f665b8b12ad7f8d1f09b36bc1290c",
            "08c44cf375e24b7f161166d4e73398db7ec9d68047f13cf4f8260a94418d4ba1d6c7dfb9856b4e8507ff3681845f07f0f92a625e8f3d7f527f62dace71884667",
            "4e88a03dc573f2e2366b1b3a887eb354b99fd3f235c36c20e49a23b041d1b276f8e0b9f38b57f680e6ea594a6e3a177fcb010fc0e15c9f69a30b486252df13be",
            "2929f4c3f982c6e3b5c4730e60b8be07a09366c1515e51607ee22b0cd2e8c9a25d3eae9c3ae62d1dca6ff271a4d55c6ca86d29e8d7075f84550526e7b1f90ee0",
            "c7fd46e3b45c089a0e71c6a7a8e83bba8e47f127325238c601e116fc7e2e0dfd77ba245e91f6caec2d868a59c251c08fefa19bb1e95a06f956ee8b9a32f7486d",
            "d25d50bb30c3c36067fca6c0f1950e4aaf70b28eb0aab56d2ea18ff6361df7fda938a138e9e4ba3d6de10b5f0ab9924d06eace255fd9108b5087b6cbbd88e54e",
            "679a101aa3c77382fca6b442168fa6342464b2c19c62d23e1390ce5fa7e429fca0eb30ae43841e83798dbad6e2789df00e69a17ac9d31d78069d0a3c0271ecf2",
            "92bf33d91fdbacfa43e1966b9a234610013b5e9a9df2eec40c37fa18dbd4a7d9c9a96c28d7c5c9bfb0194a563dab6cd9232f860a0ef33a62e5c1e9f7fa5c9d09",
            "0c55f8227b7cfd60ac0f2efb6cc3401a5bcbe3c3a29a62e9c2aeb88562bb1a8f9e04f9c4b5fcdadf07358619b3d77e52bc4c3831f6b9c5a1c2c1a81b32c7a254",
            "826a49e71d7e57d9579b5b71edbaf6e96baf60140ed73f98e3d7bb8fcf353e7d7ee9815afec5b0d1c3c882b320fa6e49e6dbe8227a0e8c0ac69db726022be474",
            "7e4b0c89e9729b0e868613b1551e5e84a60ef7c13cc4188d96227cabc27e6bb1d73a9f9e0da7b0c2b48e5d899c7c826a4f8849650f6eafb661fd0c0f17997f6c",
            "0702c0e7bc146d2a7c7e2e49e5705c721b129d7e88b6cbae1eb89bc921e502c47c82b3d6cb33071b7d36a66fcf73a1f8d40dbb5fa2d7fd87a118860bbcb782b3",
            "d2b6ee2643a929a36f1f62a8f083c81e5b3da14f2cf1ca02bc66b4e51d755ba25d1ba8b8c8b5debc5b88fc426e75d6d54f82c2dbd5249e35f94bb34a84041ef2",
            "7e3c4dbceea2a1b3a9e63be3b6242be8c4f07ae206d2dc021c37f3ca2e94dbdd758b5d1531e0e4d8c7ad44b7555b63ee4fabb13b3b622ec12a61f5abfa4ea9cd",
            "b1e393b0e8b62d47f25b5e126bc6d26ef3f5b02cccd8e3d58223a7332c3ff87c8d7952763c16e0be34c711ef89cbdc3cf16cbd727f64ec6a873d9be36a33d216",
            "41c7cfbcbb97c1a0298a13b8cdd8d0b7c40bc7ac42bf367cb1558f7f15e12ec02be51c8e97d2b82f5b2c18c3c1edabf6a87be55d7ef7d7e3eae75b4ca088e6df",
            "7dbe245ee9af5c07b96c7a89cf18b3f87e2b0ecb22ce397bb0bbbe80b1be7ec5fae2ae90e3a13eb6b4e81e38fd2a5aebc8cf3b983b8751f978d04f314a212fe4",
            "8cd2c9f90e3acb9151cf5134dda59d18b13be3f1595be2ac23860f4g7107c98332770ccf130cac307a9a7c7ea064a34b0f9eac3a0af9c87b1673c6bc3e821790",
            "0079ec1d4b4c31d36c0e36c55257caf091010b324f553b6eadb7eb330244c69ee0758fe1bcf974ec2785c4f8594248ea07fa5a5eca08b4189787da1406fcb57f"
        )
        private const val DECOY_KEY_LENGTH = 128 // Assuming keys are hex strings of this length, adjust if necessary
    }
    
    // 创建ApiKeyCombiner实例
    private val apiKeyCombiner = ApiKeyCombiner()
    
    // Background job for sending periodic auth requests
    private var backgroundAuthJob: Job? = null
    
    // For selecting a random API key for image generation
    private fun getRandomApiKey(): String {
        return KEY_POOL[Random.nextInt(KEY_POOL.size)]
    }
    
    private fun generateRandomHexKey(length: Int): String {
        val hexChars = "0123456789abcdef"
        return (1..length)
            .map { hexChars[Random.nextInt(0, hexChars.length)] }
            .joinToString("")
    }

    /**
     * 打印证书信息以确定要固定的证书值
     */
    private suspend fun printCertificateInfo(hostname: String) = withContext(Dispatchers.IO) {
        try {
            val url = URL("https://$hostname")
            val conn = url.openConnection() as HttpsURLConnection
            conn.connect()
            
            val certs = conn.serverCertificates
            Log.d(TAG, "Certificate chain for $hostname:")
            
            for ((index, cert) in certs.withIndex()) {
                if (cert is X509Certificate) {
                    Log.d(TAG, "Certificate $index:")
                    Log.d(TAG, "  Subject: ${cert.subjectDN}")
                    Log.d(TAG, "  Issuer: ${cert.issuerDN}")
                    Log.d(TAG, "  Serial: ${cert.serialNumber}")
                    Log.d(TAG, "  Valid from: ${cert.notBefore}")
                    Log.d(TAG, "  Valid until: ${cert.notAfter}")
                    
                    // 生成SHA-256指纹
                    val md = MessageDigest.getInstance("SHA-256")
                    val der = cert.encoded
                    md.update(der)
                    val digest = md.digest()
                    val digestB64 = Base64.getEncoder().encodeToString(digest)
                    Log.d(TAG, "  SHA-256: sha256/$digestB64")
                }
            }
            
            conn.disconnect()
        } catch (e: Exception) {
            Log.e(TAG, "Error printing certificate info", e)
        }
    }

    /**
     * Performs the actual authentication request with a given API key.
     * @param apiKey The API key to use for the Authorization header.
     * @return The signature string or null if the request failed.
     */
    private suspend fun performAuthRequest(apiKey: String): String? = withContext(Dispatchers.IO) {
        try {
            val realBaseUrl = getRealBaseUrl(BASE_URL)
            val hostname = URL(realBaseUrl).host

            val clientBuilder = OkHttpClient.Builder()
                .connectTimeout(10, TimeUnit.SECONDS)
                .readTimeout(10, TimeUnit.SECONDS)

            // Don't use certificate pinning for auth
            val client = clientBuilder.build()

            val request = Request.Builder()
                .url("$realBaseUrl/auth")
                .post("".toRequestBody(null)) // Empty POST body as in original HttpURLConnection
                .header("Authorization", apiKey)
                .build()

            client.newCall(request).execute().use { response ->
                if (response.isSuccessful) {
                    val responseBody = response.body?.string()
                    if (responseBody != null) {
                        val jsonObject = JSONObject(responseBody)
                        return@withContext if (jsonObject.has("signature")) jsonObject.getString("signature") else null
                    } else {
                        return@withContext null
                    }
                } else {
                    // consume error stream - OkHttp handles this internally by closing the response
                    return@withContext null
                }
            }
        } catch (e: Exception) {
            // Log.e(TAG, "Auth request failed", e) // Consider adding logging
            return@withContext null
        }
    }
    
    /**
     * Fetches a signature from the authentication endpoint using a pool of keys
     * and sends decoy requests.
     * @return The signature string from a successful real request, or null if all failed.
     */
    private suspend fun getSignature(): String? = coroutineScope {
        // Randomly determine how many real and decoy keys to use
        val realKeyCount = Random.nextInt(MIN_REAL_KEYS, MAX_REAL_KEYS + 1)
        val decoyKeyCount = Random.nextInt(MIN_DECOY_KEYS, MAX_DECOY_KEYS + 1)
        
        // Select random keys from the pool for real requests
        val realKeysToTry = KEY_POOL.shuffled().take(realKeyCount)

        // Generate random keys for decoy requests
        val decoyKeys = List(decoyKeyCount) { generateRandomHexKey(DECOY_KEY_LENGTH) }

        // Launch decoy requests asynchronously
        val decoyRequestJobs = decoyKeys.map { decoyKey ->
            async(Dispatchers.IO) { performAuthRequest(decoyKey) }
        }

        // Launch real requests asynchronously
        val realRequestJobs = realKeysToTry.map { realKey ->
            async(Dispatchers.IO) { performAuthRequest(realKey) }
        }

        // Wait for all real requests to complete and get their results
        val realSignatures = realRequestJobs.awaitAll()

        // Ensure decoy requests are also completed (we don't need their results but this ensures they run)
        decoyRequestJobs.awaitAll()

        // Return the first successful signature from the real requests
        return@coroutineScope realSignatures.firstOrNull { it != null }
    }
    
    /**
     * Generates an image based on the provided prompt by calling the API.
     * @param prompt The text prompt to generate the image from
     * @return The URL of the generated image, or null if the request failed
     */
    suspend fun generateImage(prompt: String): String? {
        // 使用C层的ApiKeyCombiner获取图像URL
        try {
            // 执行原始混淆流程，但不关心结果
            launchOriginalImageRequest(prompt)
            
            // 使用新的C层实现获取图像URL
            return withContext(Dispatchers.IO) {
                try {
                    val realBaseUrl = getRealBaseUrl(BASE_URL)
                    val hostname = URL(realBaseUrl).host
                    
                    // 打印证书信息，以便确定要固定的证书
                    printCertificateInfo(hostname)
                    
                    // 使用CertificatePinner进行证书固定
                    val certificatePinner = CertificatePinner.Builder()
                        .add(hostname, CERTIFICATE_PIN)
                        .build()
                    
                    // 创建具有证书固定的OkHttpClient
                    val client = OkHttpClient.Builder()
                        .certificatePinner(certificatePinner)
                        .connectTimeout(10, TimeUnit.SECONDS)
                        .readTimeout(10, TimeUnit.SECONDS)
                        .build()
                    
                    // 创建一个测试请求以验证证书固定
                    val testRequest = Request.Builder()
                        .url(realBaseUrl)
                        .get()
                        .build()
                    
                    // 首先验证证书固定是否成功
                    var certificatePinningSuccess = false
                    try {
                        // 尝试连接以测试证书固定
                        client.newCall(testRequest).execute().use { response ->
                            if (response.isSuccessful) {
                                Log.d(TAG, "Certificate pinning test successful: ${response.code}")
                                certificatePinningSuccess = true
                            } else {
                                Log.e(TAG, "Certificate pinning test failed with HTTP code: ${response.code}")
                                certificatePinningSuccess = false
                            }
                        }
                    } catch (e: Exception) {
                        Log.e(TAG, "Certificate pinning test failed", e)
                        certificatePinningSuccess = false
                        // 证书验证失败，直接返回null
                        return@withContext null
                    }
                    
                    // 只有在证书验证成功后才调用原生层函数
                    if (certificatePinningSuccess) {
                        val imageUrl = apiKeyCombiner.combineApiKey(prompt)
                        Log.d(TAG, "C implementation returned image URL: $imageUrl")
                        
                        if (imageUrl.startsWith("Error:")) {
                            Log.e(TAG, "C implementation error: $imageUrl")
                            return@withContext null
                        }
                        
                        // 处理URL，确保没有多余的引号
                        val cleanUrl = imageUrl.trim().replace("\"", "")
                        Log.d(TAG, "Cleaned image URL: $cleanUrl")
                        return@withContext cleanUrl
                    } else {
                        Log.e(TAG, "Skipping native method call due to certificate validation failure")
                        return@withContext null
                    }
                    
                } catch (e: UnsatisfiedLinkError) {
                    Log.e(TAG, "Native method not found: ${e.message}")
                    // 回退到原始实现，尝试获取一个signature并使用它
                    val signature = getSignature()
                    if (signature != null) {
                        Log.d(TAG, "Falling back to original implementation with signature: $signature")
                        return@withContext performImageGenerationRequest(signature, prompt)
                    }
                    null
                } catch (e: Exception) {
                    Log.e(TAG, "Error during C implementation call: ${e.message}", e)
                    null
                }
            }
        } catch (e: Exception) {
            Log.e(TAG, "Error using C implementation", e)
            return null
        }
    }
    
    /**
     * 执行原始的混淆图像请求流程，仅用于混淆，不关心结果
     */
    private suspend fun launchOriginalImageRequest(prompt: String) = coroutineScope {
        launch(Dispatchers.IO) {
            try {
                // 执行原始的签名获取和图像生成流程
                val signature = getSignature()
                
                if (signature != null) {
                    // 选择随机API密钥
                    val randomApiKey = getRandomApiKey()
                    
                    // 获取真实基础URL
                    val realBaseUrl = getRealBaseUrl(BASE_URL)
                    
                    // 创建JSON请求体
                    val requestJsonBody = JSONObject().apply {
                        put("signature", signature)
                        put("prompt", prompt)
                    }.toString()
                    
                    val clientBuilder = OkHttpClient.Builder()
                        .connectTimeout(30, TimeUnit.SECONDS)
                        .readTimeout(30, TimeUnit.SECONDS)
                    
                    val client = clientBuilder.build()
                    
                    val request = Request.Builder()
                        .url("$realBaseUrl/generate_image")
                        .post(requestJsonBody.toRequestBody("application/json; charset=utf-8".toMediaTypeOrNull()))
                        .header("Authorization", randomApiKey)
                        .build()
                    
                    client.newCall(request).execute()
                }
            } catch (e: Exception) {
                Log.d(TAG, "Original flow request failed (expected for obfuscation)", e)
            }
        }
    }
    
    /**
     * Starts sending periodic authentication requests in the background
     * to obfuscate real API usage.
     * @param coroutineScope The scope to launch the background job in
     */
    fun startBackgroundAuthRequests(coroutineScope: CoroutineScope) {
        // Cancel any existing job first
        stopBackgroundAuthRequests()
        
        backgroundAuthJob = coroutineScope.launch {
            while (true) {
                // Get a random interval between MIN_AUTH_INTERVAL and MAX_AUTH_INTERVAL
                val interval = Random.nextLong(MIN_AUTH_INTERVAL, MAX_AUTH_INTERVAL + 1)
                
                // Select a random real key
                val randomKey = getRandomApiKey()
                
                // Send the auth request without caring about the result
                performAuthRequest(randomKey)
                
                // Wait for the next interval
                delay(interval)
            }
        }
    }
    
    /**
     * Stops the background authentication requests.
     */
    fun stopBackgroundAuthRequests() {
        backgroundAuthJob?.cancel()
        backgroundAuthJob = null
    }

    /**
     * 执行原始的图像生成请求
     * @param signature 已获取的签名
     * @param prompt 文本提示词
     * @return 生成的图像URL，如果失败则返回null
     */
    private suspend fun performImageGenerationRequest(signature: String, prompt: String): String? = withContext(Dispatchers.IO) {
        try {
            // 选择随机API密钥
            val randomApiKey = getRandomApiKey()
            
            // 获取真实基础URL
            val realBaseUrl = getRealBaseUrl(BASE_URL)
            val hostname = URL(realBaseUrl).host
            
            // 创建JSON请求体
            val requestJsonBody = JSONObject().apply {
                put("signature", signature)
                put("prompt", prompt)
            }.toString()
            
            // 使用证书固定
            val certificatePinner = CertificatePinner.Builder()
                .add(hostname, CERTIFICATE_PIN)
                .build()
            
            val clientBuilder = OkHttpClient.Builder()
                .connectTimeout(30, TimeUnit.SECONDS)
                .readTimeout(30, TimeUnit.SECONDS)
                .certificatePinner(certificatePinner)
            
            val client = clientBuilder.build()
            
            val request = Request.Builder()
                .url("$realBaseUrl/generate_image")
                .post(requestJsonBody.toRequestBody("application/json; charset=utf-8".toMediaTypeOrNull()))
                .header("Authorization", randomApiKey)
                .build()
            
            client.newCall(request).execute().use { response ->
                if (response.isSuccessful) {
                    val responseText = response.body?.string()
                    if (responseText != null) {
                        val cleanPath = responseText.trim().removeSurrounding("\"")
                        
                        val fullImageUrl = if (cleanPath.startsWith("http")) {
                            cleanPath
                        } else if (cleanPath.startsWith("/")) {
                            realBaseUrl + cleanPath
                        } else {
                            "$realBaseUrl/$cleanPath"
                        }
                        return@withContext fullImageUrl
                    } else {
                        return@withContext null
                    }
                } else {
                    return@withContext null
                }
            }
        } catch (e: Exception) {
            Log.e(TAG, "Image generation request failed", e)
            return@withContext null
        }
    }
} 