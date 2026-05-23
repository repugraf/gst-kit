{
    "targets": [
        {
            "target_name": "gst_kit",
            "variables": {
                "napi_build_version%": "9",
            },
            "sources": [
                "src/cpp/addon.cpp",
                "src/cpp/async-workers.cpp",
                "src/cpp/element.cpp",
                "src/cpp/type-conversion.cpp",
                "src/cpp/pipeline.cpp",
            ],
            "dependencies": ["<!(node -p \"require('node-addon-api').gyp\")"],
            "defines": [
                "NAPI_VERSION=<(napi_build_version)",
                "NAPI_DISABLE_CPP_EXCEPTIONS",
                "BUILDING_NODE_EXTENSION",
            ],
            "cflags_cc": ["-std=c++20"],
            "xcode_settings": {
                "GCC_ENABLE_CPP_EXCEPTIONS": "NO",
                "CLANG_CXX_LIBRARY": "libc++",
                "CLANG_CXX_LANGUAGE_STANDARD": "c++20",
                "MACOSX_DEPLOYMENT_TARGET": "10.13",
            },
            "msvs_settings": {
                "VCCLCompilerTool": {
                    "LanguageStandard": "stdcpp20",
                },
                "VCLinkerTool": {
                    "SetChecksum": "true",
                    "AdditionalLibraryDirectories": [
                        "<!@(node -p \"require('child_process').execSync('pkg-config --libs-only-L gstreamer-1.0 gstreamer-app-1.0 gstreamer-rtp-1.0 glib-2.0 gobject-2.0').toString().trim().split('-L').slice(1).map(f => f.trim().replace(/\\\\\\\\ /g, ' ')).join(';')\")"
                    ],
                },
            },
            "include_dirs": [
                "<!@(node -p \"require('node-addon-api').include\")",
                ".",
                "<!@(node -p \"require('child_process').execSync('pkg-config --cflags-only-I gstreamer-1.0').toString().trim().split(/\\s+/).map(f => f.replace(/^-I/, '')).join(' ')\")",
                "<!@(node -p \"require('child_process').execSync('pkg-config --cflags-only-I gstreamer-app-1.0').toString().trim().split(/\\s+/).map(f => f.replace(/^-I/, '')).join(' ')\")",
                "<!@(node -p \"require('child_process').execSync('pkg-config --cflags-only-I gstreamer-rtp-1.0').toString().trim().split(/\\s+/).map(f => f.replace(/^-I/, '')).join(' ')\")",
                "<!@(node -p \"require('child_process').execSync('pkg-config --cflags-only-I glib-2.0').toString().trim().split(/\\s+/).map(f => f.replace(/^-I/, '')).join(' ')\")",
                "<!@(node -p \"require('child_process').execSync('pkg-config --cflags-only-I gobject-2.0').toString().trim().split(/\\s+/).map(f => f.replace(/^-I/, '')).join(' ')\")",
            ],
            "conditions": [
                [
                    "OS=='win'",
                    {
                        "libraries": [
                            "<!@(node -p \"require('child_process').execSync('pkg-config --libs-only-l gstreamer-1.0 gstreamer-app-1.0 gstreamer-rtp-1.0 glib-2.0 gobject-2.0').toString().trim().split(/\\s+/).map(f => f.replace(/^-l/, '') + '.lib').filter(f => f !== '.lib').join(' ')\")"
                        ]
                    },
                ],
                [
                    "OS!='win'",
                    {
                        "libraries": [
                            "<!@(node -p \"require('child_process').execSync('pkg-config --libs-only-l gstreamer-1.0 gstreamer-app-1.0 gstreamer-rtp-1.0 glib-2.0 gobject-2.0').toString().trim()\")"
                        ]
                    },
                ],
            ],
        }
    ]
}
