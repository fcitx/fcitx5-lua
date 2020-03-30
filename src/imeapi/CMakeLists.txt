fcitx5_translate_desktop_file(imeapi.conf.in imeapi.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/imeapi.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")

install(FILES imeapi.lua DESTINATION "${FCITX_INSTALL_PKGDATADIR}/lua/imeapi")

