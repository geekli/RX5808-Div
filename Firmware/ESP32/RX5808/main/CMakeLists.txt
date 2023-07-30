file(GLOB_RECURSE SOURCES "./*.c"
    "./hardware/*.c"
    "./gui/lvgl_app/*.c"
    "./gui/lvgl_app/resources/*.c"
    "./gui/lvgl_driver/*.c"
    "./driver/*.c"
)
set(INCLUDE_DIRSx "./"
    "./hardware/"
    "./gui/"
    "./gui/lvgl_app/"
    "./gui/lvgl_app/resources/"
    "./gui/lvgl_driver/"
    "./driver/"
)

set(SRCSx main.c)

idf_component_register(SRCS ${SRCSx} ${SOURCES} INCLUDE_DIRS ${INCLUDE_DIRSx})

# idf_component_register(SRCS "main.c" INCLUDE_DIRS "")
