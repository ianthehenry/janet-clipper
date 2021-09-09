( declare-project
  :name "clipper"
  :author "Ian Henry <ianthehenry@gmail.com>")

(declare-native
  :name "clipper"
  :cppflags [;default-cppflags "-Iclipper/"]
  :source ["src/main.cpp" "clipper/clipper.cpp"])
