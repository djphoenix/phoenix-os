***
> PhoeniX OS [![build status](https://git.phoenix.dj/phoenix/phoenix-os/badges/master/build.svg)](https://git.phoenix.dj/phoenix/phoenix-os/builds)<br>
> © DJ PhoeniX, 2012-2015<br>
> https://git.phoenix.dj/phoenix/phoenix-os

***

Данный проект представляет из себя 64-битную операционную систему с открытым исходным кодом.
Данная ОС не базируется на GNU/Linux или Microsoft® Windows®, однако может поддерживать форматы исполняемых файлов, драйверов и модулей этих ОС.

## Структура папок и файлов
В папке bin содержатся текущие актуальные версии бинарных файлов, необходимых для запуска ОС, таких как:
* bin/pxkrnl - ядро PhoeniX OS
* bin/grldr - Grub4dos (http://sourceforge.net/projects/grub4dos/)
* bin/menu.lst/default - menu.lst для Grub4dos

В папке doc содержатся файлы документации по основным частям ОС. Начать изучение документации рекомендуется с файла [doc/Boot.txt](https://git.phoenix.dj/phoenix/phoenix-os/wikis/boot)

В папке src содержатся файлы исходного кода (*.s - на языке Ассемблера, *.cpp и *.h - C/C++)

Для компиляции необходимы [MAKE](http://www.gnu.org/software/make/), [GCC](http://gcc.gnu.org/), а так же [MinGW](http://www.mingw.org/) / [Cygwin](http://www.cygwin.com/) для компиляции в среде OS Windows.

## Тестирование

Процесс установки необходимого для сборки ПО, а так же самой сборки, описан в разделе Wiki "[Сборка](https://git.phoenix.dj/phoenix/phoenix-os/wikis/build)".

Инструкции по запуску смотрите в разделах:

* [Загрузка на ПК](https://git.phoenix.dj/phoenix/phoenix-os/wikis/run-pc)
* [Загрузка на виртуальных машинах](https://git.phoenix.dj/phoenix/phoenix-os/wikis/run-vm)

## Отказ от гарантий
Данный исходный код предоставляется "как есть", разработчик не несёт ответственности за его работу.