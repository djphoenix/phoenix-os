# PhoeniX OS
***&copy; DJ PhoeniX, 2012***

***<http://goo.gl/X2e9z>***

---

## Модули типа "драйвер"
См. **doc/Modules.txt** для получения базовой информации о модулях.

## Драйверная модель
Драйверы в представлении PhoeniX OS - приложения, работающие на уровне 1 (ядро работает на уровне 0).
Коммуникация между драйверами происходит через именованные интерфейсы. При инициализации драйвер вызывает системную функцию RegisterInterface, в которой передаёт название интерфейса и функцию его обработки. При вызове интерфейса приложением или другим драйвером происходит передача указателя на структуру, которая помещена в общей памяти. Обработчик интерфейса получает указатель на структуру (первое поле которой - size типа int64). После обработки данных интерфейс должен вернуть аналогичную структуру, заполненную данными в соответствии с переданным запросом, либо int64 с произвольными данными (в зависимости от интерфейса).
Вызов интерфейса происходит в следующем порядке:
1. Расчёт длины передаваемых интерфейсу данных
2. Выделение блока памяти с размером (длина + 8 байт) (int64 - длина данных) и флагом SHARED
3. Помещение в смещение 0 длины передаваемых данных
4. Помещение данных в выделенный блок с 8 байта
5. Вызов функции InvokeInterface, которой передаются параметры "имя_драйвера", "имя_интерфейса" и "указатель_на_структуру"

Функция InvokeInterface возвращает данные из обработчика интерфейса "как есть". Если интерфейс не был найден, возвращается значение -1. Если предусмотрен аналогичный ответ от интерфейса, необходимо проверить (при помощи GetLastError) тип последней ошибки на соответствие INTERFACE_NOT_FOUND.

Если интерфейс возвращает указатель на блок памяти, то "владельцем" этого блока устанавливается вызвавшее его приложение/драйвер, и освобождение этого блока должно осуществляться именно в нём.