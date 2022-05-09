set(FILES
    Dummy.cpp
    Common.h # HLSL <-> C++ translation
    Sample.hlsl # main library
)

set(SCATTERING_SHADERS
    Scattering/SingleInscatterCompute.hlsl
    Scattering/SingleIrradianceCompute.hlsl
    Scattering/TransmittanceCompute.hlsl
)

add_library(shaders STATIC ${FILES} ${SCATTERING_SHADERS})

set_source_files_properties(Sample.hlsl
    PROPERTIES 
        VS_SHADER_MODEL "6.3"
        VS_SHADER_TYPE "Library"
        VS_SHADER_ENTRYPOINT ""
        VS_SHADER_FLAGS "/Zpr /WX" # for row-major matrices
)

foreach(SHADER IN LISTS SCATTERING_SHADERS)
    set_source_files_properties(${SHADER}
        PROPERTIES 
            VS_SHADER_MODEL "6.0"
            VS_SHADER_TYPE "Compute"
            VS_SHADER_ENTRYPOINT "CSMain"
            VS_SHADER_FLAGS "/Zpr /WX" # for row-major matrices
    )
endforeach()

set(INTERMEDIATE_DIR "${CMAKE_BINARY_DIR}/shaders/${CMAKE_CFG_INTDIR}")
set(OUTPUT_FILES 
        Sample.cso
        TransmittanceCompute.cso
        SingleInscatterCompute.cso
        SingleIrradianceCompute.cso
)

foreach(OFILE IN LISTS OUTPUT_FILES)
    add_custom_command(TARGET shaders POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E create_symlink "${INTERMEDIATE_DIR}/${OFILE}" "${CMAKE_BINARY_DIR}/dx12_sample/${OFILE}"
        COMMAND ${CMAKE_COMMAND} -E create_symlink "${INTERMEDIATE_DIR}/${OFILE}" "${CMAKE_BINARY_DIR}/dx12_sample/${CMAKE_CFG_INTDIR}/${OFILE}"
    )
endforeach()
