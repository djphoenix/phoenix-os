# PhoeniX OS
***&copy; DJ PhoeniX, 2012***

***<http://goo.gl/X2e9z>***

---

## Модули ядра PhoeniX OS
Модулем называется расширение (драйвер или программа) для PhoeniX OS.
Формат модулей произвольный, изначально поддерживаются DLL и ELF, возможно добавить (через механизм модулей) другие форматы, например, EXE или BINARY.
При загрузке модулей происходит получение метаданных:
1. Тип (программа или драйвер)
2. Название
3. Версия
4. Разработчик
5. Комментарии разработчика (например, URL сайта разработчика или описание)
6. Цифровая подпись модуля, в случае её наличия (сертификат, которым подписан модуль, должен быть подписан основным сертификатом PhoeniX OS)

Если определён тип "программа", происходит определение связанных типов данных (текстовые, графические и другие). Mime-типы определяются модулями файлового менеджера.
Если определён тип "драйвер", происходит получение списка его зависимостей. Если не все драйверы, от которого зависит загружаемый модуль, уже загружены, модуль загружается в память с флагом suspended, после чего продолжается загрузка остальных модулей.
Описание процесса инициализации драйверов см. файле **doc/Drivers.txt**
После загрузки всех драйверов происходит проверка всех модулей с флагом suspended. Если таковые найдены, их зависимости проверяются снова. Если зависимости удовлетворены, происходит инициализация модуля и флаг suspended снимается, при этом увеличивается значения счётчика. После очередного прохода происходит проверка счётчика. Если он равен 0 (все модули инициализированы, либо оставшиеся модули не могут быть запущены из-за зависимостей), происходит выход из процесса загрузки модулей, иначе проход по списку запускается заново, при этом счётчик обнуляется.

## Ручная загрузка/выгрузка
Модуль может быть загружен или выгружен вручную через приложение "Диспетчер модулей". При загрузке проверяются зависимости модуля, если одного из модулей не загружено - происходит ошибка, и пользователю предлагается загрузить недостающий модуль вручную (при отсутствии модуля на диске и наличии сети модуль автоматически проверяется в репозитории и предлагается ссылка на его скачивание). При выгрузке проверяются зависимости других модулей от выгружаемого и предлагается выгрузить все зависимые модули.

## Сбои в модулях
Каждый модуль работает только со своими ресурсами. При сбое в модуле, происходит освобождение всех используемых ресурсов и попытка его перезапуска. Если есть модули, зависимые от ошибочного, в них так же происходит процесс сброса.

## Конфликты модулей
Если происходит попытка загрузки модулей, отвечающих за ресурсы одного типа (например, модули интерфейса пользователя, или драйверы одного устройства разных версий), то, в зависимости от вида загрузки (автоматическая или ручная), происходит либо отмена загрузки, либо пользователю предлагается выгрузить старый драйвер перед загрузкой нового. При этом все зависимые драйверы будут проверены на совместимость и, при необходимости, несовместимые будут выгружены, а остальные сброшены.

## Логика зависимостей
Например, драйверы usb/uhci, usb/ehci и usb/ohci предоставляют общий класс usb, однако отвечают за разные устройства. В таком случае, корневой модуль usb имеет зависимость вида "usb/uhci OR usb/ehci OR usb/ohci", предоставляя общий интерфейс доступа к устройствам USB, и проверяет доступные модули вручную, однако он не будет загружен, если нет ни одного зависимого драйвера.
Так же, доступны и другие виды зависимостей, например, ONE (один из нескольких), AND (аналогичный простому перечислению).