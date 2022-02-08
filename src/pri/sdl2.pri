
win32: {
INCLUDEPATH += $$(SDL2)/include
LIBS += -L"$$(SDL2)/lib/x86"
}

unix: {
    INCLUDEPATH += /usr/local/include/SDL2
    LIBS += -lSDL2 -lSDL2main
}
