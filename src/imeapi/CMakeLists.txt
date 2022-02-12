configure_file(imeapi.conf.in.in imeapi.conf.in)
fcitx5_translate_desktop_file("${CMAKE_CURRENT_BINARY_DIR}/imeapi.conf.in" imeapi.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/imeapi.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon"
        COMPONENT config)

install(FILES imeapi.lua DESTINATION "${FCITX_INSTALL_PKGDATADIR}/lua/imeapi"
        COMPONENT config)
