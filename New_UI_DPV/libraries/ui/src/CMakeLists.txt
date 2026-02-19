SET(SOURCES ui_Screen1.c
    ui.c
    ui_comp_hook.c
    ui_helpers.c
    ui_img_img_final_png.c
    fonts/ui_font_DPV128.c
    fonts/ui_font_DPV220.c
    fonts/ui_font_DPV256.c
    fonts/ui_font_DPV80.c
    fonts/ui_font_DPV80new.c
    fonts/ui_font_Font1.c)

add_library(ui ${SOURCES})
