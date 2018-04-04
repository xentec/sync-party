include(UseLATEX)

add_latex_document(plakat/plakat.tex FORCE_PDF)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/plakat.pdf DESTINATION docs)
