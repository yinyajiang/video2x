cmake_minimum_required(VERSION 3.10 FATAL_ERROR)
project(image2x)

include(CMakePackageConfigHelpers)
include(ExternalProject)
include(GNUInstallDirs)

# The FindBoost module is removed in CMake 3.30
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# Set the default build type to Release if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Build options
option(IMAGE2X_BUILD_SHARED_LIBS "Build libimage2x as a shared library" ON)
option(BUILD_SHARED_LIBS "Build libimage2x as a shared library" ${IMAGE2X_BUILD_SHARED_LIBS})
option(IMAGE2X_USE_EXTERNAL_NCNN "Use the system-provided ncnn library" OFF)
option(IMAGE2X_USE_MAC_NCNN_SOURCE "Use mac ncnn source" OFF)
option(IMAGE2X_BUILD_IMAGE2X_CLI "build image2x cli" ON)

option(IMAGE2X_ENABLE_NATIVE "Enable optimizations for the native architecture" OFF)
option(IMAGE2X_ENABLE_X86_64_V4 "Enable x86-64-v4 (AVX-512) optimizations" OFF)
option(IMAGE2X_ENABLE_X86_64_V3 "Enable x86-64-v3 (AVX2) optimizations" OFF)

# Enable extra compiler warnings
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/W4 /permissive-)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wconversion -Wshadow)
endif()

# Set the default optimization flags for Release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        add_compile_options(/Ox /Ot /GL /DNDEBUG)
        add_link_options(/LTCG /OPT:REF /OPT:ICF)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-O3 -ffunction-sections -fdata-sections)
        if(APPLE)
            add_link_options(-Wl,-s -flto -Wl,-dead_strip)
        else()
            add_link_options(-Wl,-s -flto -Wl,--gc-sections)
        endif()
    endif()
endif()

# Enable the requested architecture-specific optimizations
if(IMAGE2X_ENABLE_NATIVE)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        add_compile_options(/arch:NATIVE)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-march=native)
    endif()
elseif(IMAGE2X_ENABLE_X86_64_V4)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        add_compile_options(/arch:AVX512)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-march=x86-64-v4)
    endif()
elseif(IMAGE2X_ENABLE_X86_64_V3)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        add_compile_options(/arch:AVX2)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-march=x86-64-v3)
    endif()
endif()


# Add the libimage2x shared library
add_library(libimage2x
    src/libimage2x.cpp
    src/imgconversions.cpp
    src/imgfilter_realcugan.cpp
    src/imgfilter_realesrgan.cpp
    src/gpu_utils.cpp
    src/fileutils.cpp
)

# Set the C++ standard to C++17
target_compile_features(libimage2x PRIVATE cxx_std_17)

# Set the shared library output name and disable C++ extensions
set_target_properties(libimage2x PROPERTIES
    PREFIX ""
    OUTPUT_NAME libimage2x
    CXX_EXTENSIONS OFF
)

# Include directories for the shared library
target_include_directories(libimage2x PRIVATE
    "${CMAKE_CURRENT_BINARY_DIR}"
    "./include"
    "../../third_party/librealesrgan_ncnn_vulkan/src"
    "../../third_party/librealcugan_ncnn_vulkan/src"
    "../../third_party/librife_ncnn_vulkan/src"
    "../../include/libvideo2x"
)

# Add the export definition for the exported symbols
target_compile_definitions(libimage2x PRIVATE LIBIMAGE2X_EXPORTS)

# Set debug compile options
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(libimage2x PRIVATE $<$<CONFIG:Debug>:/Zi /Od /MDd>)
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(libimage2x PRIVATE
        $<$<NOT:$<PLATFORM_ID:Windows>>:-fPIC>
        $<$<CONFIG:Debug>:-g -O0>
    )
endif()

# ncnn
if(NOT TARGET ncnn)
    if(IMAGE2X_USE_EXTERNAL_NCNN)
        find_package(ncnn REQUIRED)
    else()
        if(WIN32)
            # Use the pre-built shared ncnn library on Windows
            set(ncnn_base_path "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/ncnn-shared/x64")
            add_library(ncnn SHARED IMPORTED)
            set_target_properties(ncnn PROPERTIES
                IMPORTED_LOCATION "${ncnn_base_path}/bin/ncnn.dll"
                IMPORTED_IMPLIB "${ncnn_base_path}/lib/ncnn.lib"
                INTERFACE_INCLUDE_DIRECTORIES "${ncnn_base_path}/include/ncnn"
            )
        elseif(NOT IMAGE2X_USE_MAC_NCNN_SOURCE)
            set(ncnn_base_path "${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/ncnn-shared-mac")
            add_library(ncnn SHARED IMPORTED)
            set_target_properties(ncnn PROPERTIES
                IMPORTED_LOCATION "${ncnn_base_path}/ncnn.framework/ncnn"
                INTERFACE_INCLUDE_DIRECTORIES "${ncnn_base_path}/ncnn.framework/Headers/ncnn"
                INTERFACE_LINK_LIBRARIES "${ncnn_base_path}/glslang.framework;${ncnn_base_path}/openmp.framework"
                FRAMEWORK TRUE
            )
        else()
            option(NCNN_INSTALL_SDK "" ON)
            option(SKIP_GLSLANG_INSTALL "" OFF)

            option(NCNN_PIXEL_ROTATE "" OFF)
            option(NCNN_PIXEL_AFFINE "" OFF)
            option(NCNN_PIXEL_DRAWING "" OFF)
            option(NCNN_VULKAN "" ON)
            option(NCNN_VULKAN_ONLINE_SPIRV "" ON)
            option(NCNN_BUILD_BENCHMARK "" OFF)
            option(NCNN_BUILD_TESTS "" OFF)
            option(NCNN_BUILD_TOOLS "" OFF)
            option(NCNN_BUILD_EXAMPLES "" OFF)
            option(NCNN_DISABLE_RTTI "" ON)
            option(NCNN_DISABLE_EXCEPTION "" ON)
            option(NCNN_INT8 "" OFF)
            option(NCNN_BUILD_SHARED_LIBS "" OFF)

            option(WITH_LAYER_absval "" OFF)
            option(WITH_LAYER_argmax "" OFF)
            option(WITH_LAYER_batchnorm "" OFF)
            option(WITH_LAYER_bias "" OFF)
            option(WITH_LAYER_bnll "" OFF)
            option(WITH_LAYER_concat "" ON)
            option(WITH_LAYER_convolution "" ON)
            option(WITH_LAYER_crop "" ON)
            option(WITH_LAYER_deconvolution "" ON)
            option(WITH_LAYER_dropout "" OFF)
            option(WITH_LAYER_eltwise "" ON)
            option(WITH_LAYER_elu "" OFF)
            option(WITH_LAYER_embed "" OFF)
            option(WITH_LAYER_exp "" OFF)
            option(WITH_LAYER_flatten "" ON)
            option(WITH_LAYER_innerproduct "" ON)
            option(WITH_LAYER_input "" ON)
            option(WITH_LAYER_log "" OFF)
            option(WITH_LAYER_lrn "" OFF)
            option(WITH_LAYER_memorydata "" ON)
            option(WITH_LAYER_mvn "" OFF)
            option(WITH_LAYER_pooling "" ON)
            option(WITH_LAYER_power "" OFF)
            option(WITH_LAYER_prelu "" ON)
            option(WITH_LAYER_proposal "" OFF)
            option(WITH_LAYER_reduction "" ON)
            option(WITH_LAYER_relu "" ON)
            option(WITH_LAYER_reshape "" ON)
            option(WITH_LAYER_roipooling "" OFF)
            option(WITH_LAYER_scale "" ON)
            option(WITH_LAYER_sigmoid "" ON)
            option(WITH_LAYER_slice "" ON)
            option(WITH_LAYER_softmax "" OFF)
            option(WITH_LAYER_split "" ON)
            option(WITH_LAYER_spp "" OFF)
            option(WITH_LAYER_tanh "" OFF)
            option(WITH_LAYER_threshold "" OFF)
            option(WITH_LAYER_tile "" OFF)
            option(WITH_LAYER_rnn "" OFF)
            option(WITH_LAYER_lstm "" OFF)
            option(WITH_LAYER_binaryop "" ON)
            option(WITH_LAYER_unaryop "" ON)
            option(WITH_LAYER_convolutiondepthwise "" OFF)
            option(WITH_LAYER_padding "" ON)
            option(WITH_LAYER_squeeze "" OFF)
            option(WITH_LAYER_expanddims "" OFF)
            option(WITH_LAYER_normalize "" OFF)
            option(WITH_LAYER_permute "" OFF)
            option(WITH_LAYER_priorbox "" OFF)
            option(WITH_LAYER_detectionoutput "" OFF)
            option(WITH_LAYER_interp "" ON)
            option(WITH_LAYER_deconvolutiondepthwise "" OFF)
            option(WITH_LAYER_shufflechannel "" OFF)
            option(WITH_LAYER_instancenorm "" OFF)
            option(WITH_LAYER_clip "" ON)
            option(WITH_LAYER_reorg "" OFF)
            option(WITH_LAYER_yolodetectionoutput "" OFF)
            option(WITH_LAYER_quantize "" OFF)
            option(WITH_LAYER_dequantize "" OFF)
            option(WITH_LAYER_yolov3detectionoutput "" OFF)
            option(WITH_LAYER_psroipooling "" OFF)
            option(WITH_LAYER_roialign "" OFF)
            option(WITH_LAYER_packing "" ON)
            option(WITH_LAYER_requantize "" OFF)
            option(WITH_LAYER_cast "" ON)
            option(WITH_LAYER_hardsigmoid "" OFF)
            option(WITH_LAYER_selu "" OFF)
            option(WITH_LAYER_hardswish "" OFF)
            option(WITH_LAYER_noop "" OFF)
            option(WITH_LAYER_pixelshuffle "" ON)
            option(WITH_LAYER_deepcopy "" OFF)
            option(WITH_LAYER_mish "" OFF)
            option(WITH_LAYER_statisticspooling "" OFF)
            option(WITH_LAYER_swish "" OFF)
            option(WITH_LAYER_gemm "" OFF)
            option(WITH_LAYER_groupnorm "" OFF)
            option(WITH_LAYER_layernorm "" OFF)
            option(WITH_LAYER_softplus "" OFF)
            option(WITH_LAYER_gru "" OFF)
            option(WITH_LAYER_multiheadattention "" OFF)
            option(WITH_LAYER_gelu "" OFF)
            option(WITH_LAYER_convolution1d "" OFF)
            option(WITH_LAYER_pooling1d "" OFF)
            option(WITH_LAYER_convolutiondepthwise1d "" OFF)
            option(WITH_LAYER_convolution3d "" OFF)
            option(WITH_LAYER_convolutiondepthwise3d "" OFF)
            option(WITH_LAYER_pooling3d "" OFF)
            option(WITH_LAYER_matmul "" OFF)
            option(WITH_LAYER_deconvolution1d "" OFF)
            option(WITH_LAYER_deconvolutiondepthwise1d "" OFF)
            option(WITH_LAYER_deconvolution3d "" OFF)
            option(WITH_LAYER_deconvolutiondepthwise3d "" OFF)
            option(WITH_LAYER_einsum "" OFF)
            option(WITH_LAYER_deformableconv2d "" OFF)
            option(WITH_LAYER_gridsample "" ON)

            add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/ncnn ${CMAKE_CURRENT_BINARY_DIR}/third_party/ncnn)
        endif()
    endif()
endif()
target_link_libraries(libimage2x PRIVATE ncnn)


# Add Real-ESRGAN, Real-CUGAN, and RIFE
option(USE_SYSTEM_NCNN "" ${IMAGE2X_USE_EXTERNAL_NCNN})
if(NOT TARGET librealesrgan-ncnn-vulkan)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/librealesrgan_ncnn_vulkan/src ${CMAKE_CURRENT_BINARY_DIR}/third_party/librealesrgan_ncnn_vulkan)
endif()
if(NOT TARGET librealcugan-ncnn-vulkan)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/librealcugan_ncnn_vulkan/src ${CMAKE_CURRENT_BINARY_DIR}/third_party/librealcugan_ncnn_vulkan)
endif()
if(NOT TARGET librife-ncnn-vulkan)
    add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/librife_ncnn_vulkan/src ${CMAKE_CURRENT_BINARY_DIR}/third_party/librife_ncnn_vulkan)
endif()

# Prevent the min and max macros from causing error C2589 on Windows
if(WIN32)
    target_compile_definitions(librealesrgan-ncnn-vulkan PRIVATE -DNOMINMAX)
    target_compile_definitions(librealcugan-ncnn-vulkan PRIVATE -DNOMINMAX)
    target_compile_definitions(librife-ncnn-vulkan PRIVATE -DNOMINMAX)
endif()

# Link the shared library to the ncnn-Vulkan libraries
target_link_libraries(libimage2x PRIVATE
    librealesrgan-ncnn-vulkan
    librealcugan-ncnn-vulkan
    librife-ncnn-vulkan
)

# Common installation rules for libimage2x and models
install(TARGETS libimage2x
    EXPORT Video2XTargets
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/libimage2x"
)

# Install the header files from the include directory
install(DIRECTORY "../../include/libimage2x/"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/libimage2x"
    FILES_MATCHING PATTERN "libimage2x.h"
)

# Install the model files
if(WIN32)
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../../models" DESTINATION "${CMAKE_INSTALL_DATADIR}")
else()
    install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../../models" DESTINATION "${CMAKE_INSTALL_DATADIR}/libimage2x")
endif()

# Install ncnn shared libraries
if(WIN32)
    if(NOT IMAGE2X_USE_EXTERNAL_NCNN)
        install(FILES  "${ncnn_base_path}/bin/ncnn.dll" DESTINATION "${CMAKE_INSTALL_BINDIR}")
    endif()
endif()

# Vulkan
find_package(Vulkan REQUIRED)
target_link_libraries(libimage2x PRIVATE Vulkan::Vulkan)


if(IMAGE2X_BUILD_IMAGE2X_CLI)
    # image2x cli
    add_executable(image2x
            src/image2x.cpp
    )
    # Set the C++ standard to C++17
    target_compile_features(image2x PRIVATE cxx_std_17)
    # Set the shared library output name and disable C++ extensions
    set_target_properties(image2x PROPERTIES
            OUTPUT_NAME image2x
            CXX_EXTENSIONS OFF
    )
    # Include directories for the executable
    target_include_directories(image2x PRIVATE
            "${CMAKE_CURRENT_BINARY_DIR}"
            "include"
            "../../third_party/librealesrgan_ncnn_vulkan/src"
            "../../third_party/librealcugan_ncnn_vulkan/src"
            "../../third_party/librife_ncnn_vulkan/src"
    )

    # Link the executable with the shared library
    target_link_libraries(image2x PRIVATE libimage2x)

    # Set debug compile options
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        target_compile_options(image2x PRIVATE $<$<CONFIG:Debug>:/Zi /Od /MDd>)
    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_compile_options(image2x PRIVATE $<$<CONFIG:Debug>:-g -O0>)
    endif()

    # Boost
    if(NOT TARGET boost_accumulators)
        if(VIDEO2X_USE_EXTERNAL_BOOST)
            find_package(Boost REQUIRED COMPONENTS program_options)
        else()
            option(Boost_USE_STATIC_LIBS "" ON)
            option(Boost_USE_STATIC_RUNTIME "" ON)
            option(Boost_COMPONENTS program_options)

            add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/boost ${CMAKE_CURRENT_BINARY_DIR}/third_party/boost)
            target_include_directories(image2x SYSTEM PRIVATE
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/boost/libs/program_options/include
                    ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party/boost/libs/config/include
            )
            # Suppress the -Wsign-conversion warnings for Boost.Nowide
            if (TARGET boost_nowide AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
                target_compile_options(boost_nowide PRIVATE -Wno-error=sign-conversion)
            endif()

            target_link_libraries(image2x PRIVATE Boost::program_options)
        endif()
    endif()
endif()