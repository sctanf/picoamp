if (NOT TARGET sctanf_i2s) # change the name so it doesnt try to use the original i2s stuffs
    add_library(sctanf_i2s INTERFACE)

    pico_generate_pio_header(sctanf_i2s ${CMAKE_CURRENT_LIST_DIR}/audio_i2s.pio)

    target_sources(sctanf_i2s INTERFACE
            ${CMAKE_CURRENT_LIST_DIR}/audio_i2s.c
    )

    target_include_directories(sctanf_i2s INTERFACE ${CMAKE_CURRENT_LIST_DIR}/include)
    target_link_libraries(sctanf_i2s INTERFACE hardware_dma hardware_pio hardware_irq pico_audio)
endif()