# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/Lucian/esp/esp-idf/components/bootloader/subproject"
  "C:/Users/Lucian/n2120915/Robo_Hand/Studienarbeit_Hand/VSC_MyoWareWireless/MyowareWireless_Reciever/build/bootloader"
  "C:/Users/Lucian/n2120915/Robo_Hand/Studienarbeit_Hand/VSC_MyoWareWireless/MyowareWireless_Reciever/build/bootloader-prefix"
  "C:/Users/Lucian/n2120915/Robo_Hand/Studienarbeit_Hand/VSC_MyoWareWireless/MyowareWireless_Reciever/build/bootloader-prefix/tmp"
  "C:/Users/Lucian/n2120915/Robo_Hand/Studienarbeit_Hand/VSC_MyoWareWireless/MyowareWireless_Reciever/build/bootloader-prefix/src/bootloader-stamp"
  "C:/Users/Lucian/n2120915/Robo_Hand/Studienarbeit_Hand/VSC_MyoWareWireless/MyowareWireless_Reciever/build/bootloader-prefix/src"
  "C:/Users/Lucian/n2120915/Robo_Hand/Studienarbeit_Hand/VSC_MyoWareWireless/MyowareWireless_Reciever/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/Lucian/n2120915/Robo_Hand/Studienarbeit_Hand/VSC_MyoWareWireless/MyowareWireless_Reciever/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "C:/Users/Lucian/n2120915/Robo_Hand/Studienarbeit_Hand/VSC_MyoWareWireless/MyowareWireless_Reciever/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
