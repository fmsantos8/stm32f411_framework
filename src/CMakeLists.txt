target_sources(${PROJECT_NAME} PRIVATE
    main.c
    app/button/app_button.c
    app/status_led/app_status_led.c
    app/acquisition/app_acquisition.c
    app/telemetry/app_telemetry.c
    app/gui/app_gui.c
    app/gui/render.c
    app/gui/bitmaps.c
    components/button/cmp_button.c
    components/led/cmp_led.c
    components/sht31d/cmp_sht31d.c
    components/ssd1306/cmp_ssd1306.c
    components/ssd1306/cmp_ssd1306_fonts.c
    components/telemetry/cmp_telemetry.c
)
target_include_directories(${PROJECT_NAME} PRIVATE
    app
    components/button
    components/led
    components/sht31d
    components/ssd1306
    components/telemetry
)
