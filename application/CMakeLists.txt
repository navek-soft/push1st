foreach(APP ${APP_NAME})
    add_subdirectory(${APP})
endforeach(APP IN APP_NAME)
