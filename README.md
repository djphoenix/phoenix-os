***
> PhoeniX OS [![build status](https://git.phoenix.dj/ci/projects/2/status.png?ref=master)](https://git.phoenix.dj/ci/projects/2?ref=master)<br>
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

compile.bat и compile.sh - скрипты компиляции для Windows и Linux соответственно

Для компиляции необходимы GCC (http://gcc.gnu.org/) и FASM (http://flatassembler.net/)

## Тестирование

Процесс установки необходимого для сборки ПО, а так же самой сборки, описан в разделе Wiki "[Сборка](https://git.phoenix.dj/phoenix/phoenix-os/wikis/build)".

Инструкции по запуску смотрите в разделах:

* [Загрузка на ПК](https://git.phoenix.dj/phoenix/phoenix-os/wikis/run-pc)
* [Загрузка на виртуальных машинах](https://git.phoenix.dj/phoenix/phoenix-os/wikis/run-vm)

## Отказ от гарантий
Данный исходный код предоставляется "как есть", разработчик не несёт ответственности за его работу.