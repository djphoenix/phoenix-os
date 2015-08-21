# PhoeniX OS [![Build Status](https://travis-ci.org/djphoenix/PhoeniXOS.svg)](https://travis-ci.org/djphoenix/PhoeniXOS)

***&copy; DJ PhoeniX, 2012***

***<http://goo.gl/X2e9z>***

---

## PhoeniX OS
Данный проект представляет из себя 64-битную операционную систему с открытым исходным кодом.
Данная ОС не базируется на GNU/Linux или Microsoft® Windows®, однако может поддерживать форматы исполняемых файлов, драйверов и модулей этих ОС.

## Структура папок и файлов
В папке bin содержатся текущие актуальные версии бинарных файлов, необходимых для запуска ОС, таких как:

* **bin/pxkrnl** - ядро PhoeniX OS
* **bin/grldr** - [Grub4dos](http://sourceforge.net/projects/grub4dos/)
* **bin/menu.lst/default** - menu.lst для Grub4dos

В папке doc содержатся файлы документации по основным частям ОС. Начать изучение документации рекомендуется с файла **doc/Boot.txt**

В папке src содержатся файлы исходного кода (\*.s - на языке Ассемблера, \*.cpp и \*.hpp - C/C++)

Для компиляции необходимы [MAKE](http://www.gnu.org/software/make/), [GCC](http://gcc.gnu.org/) и [FASM](http://flatassembler.net/), а так же [MinGW](http://www.mingw.org/) / [Cygwin](http://www.cygwin.com/) для компиляции в среде OS Windows.

## Отказ от гарантий
Данный исходный код предоставляется "как есть", разработчик не несёт ответственности за его работу.