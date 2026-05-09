plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

val version = project.properties["mod_version"] as String? ?: "1.0.0"
val versionCode = (project.properties["app_version_code"] as String? ?: "1").toInt()

android {
    namespace = "com.minefilter.launcher"
    compileSdk = project.properties["android.compileSdk"] as String?.toInt() ?: 34

    defaultConfig {
        applicationId = "com.minefilter.launcher"
        minSdk = project.properties["android.minSdk"] as String?.toInt() ?: 24
        targetSdk = project.properties["android.targetSdk"] as String?.toInt() ?: 34
        versionCode = versionCode
        versionName = version

        externalNativeBuild {
            cmake {
                cppFlags += "-std=c++17"
                arguments += "-DANDROID_STL=c++_shared"
            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
            proguardFiles(getDefaultProguardFile("proguard-android-optimize.txt"))
        }
    }

    externalNativeBuild {
        cmake {
            path = file("../native_engine/CMakeLists.txt")
            version = "3.22.1"
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    kotlinOptions {
        jvmTarget = "17"
    }
}

dependencies {
    implementation("androidx.appcompat:appcompat:${project.properties["appcompat_version"]}")
    implementation("com.google.android.material:material:${project.properties["material_version"]}")
}
