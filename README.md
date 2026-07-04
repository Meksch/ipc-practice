# ipc-practice

Два окремих CLI-додатки (Producer і Consumer), що обмінюються пакетами даних через POSIX shared memory та буфер. Використані тільки стандартні POSIX-механізми — без Boost, ZeroMQ і т.д.

### Компоненти

| Файл | Призначення |
|---------------|-------------|
| `SharedMemory` | RAII-обгортка над `shm_open` / `mmap` / `munmap` |
| `MemBuff` | Буфер у shared memory з семафорами |
| `PacketHeader` | Формат пакета (заголовок + корисне навантаження) |
| `crc32` | Чексума |
| `PauseController` | Пауза / відновлення через сигнали і клавіатуру |

### Формат пакета

```cpp
struct PacketHeader {
    uint32_t marker;        // 'IPCP' - просто маркер для перевірки
    uint32_t sequence;      //  лічильник пакетів, з 0
    uint64_t timestamp_ns;  // час надсилання
    uint32_t payload_size;  // розмір навантаження (в байтах)
    uint32_t checksum;      // CRC32
};
// слот виглядає так: [PacketHeader][payload]
```

## Білд і запуск

```bash
cmake -S . -B build
cmake --build build -j
```

Порядок запуску: спочатку Producer (створює shared memory), потім Consumer (підключається до неї).

```bash
# термінал 1
./build/producer --payload-size 4096 --slots 64

# термінал 2 — mem-name беремо з виводу producer
./build/consumer --mem-name /ipc-practice-12345
```

### Параметри

| Флаг | Додаток | Опис |
|--------|-------|------|
| `--payload-size N` | producer | Розмір навантаження у байтах |
| `--slots N` | producer | Кількість слотів. Якщо не вказаго - 1024 |
| `--mem-name NAME` | consumer | Ім'я shared memory |

## Пауза і відновлення

Кожен процес керує паузою незалежно. Поки процес на паузі, він не викликає `sem_wait` і чекає 10 мс.

### Керування

| Ввід | Дія |
|------|-----|
| `kill -USR1 <pid>` | Поставити на паузу |
| `kill -USR2 <pid>` | Відновити |
| Будь-яка клавіша в терміналі | Переключає мфіж паузою і роботою |
| `Ctrl+C` / `kill -INT <pid>` | Зупинити процес |

### Що відбувається з даними під час паузи

**Consumer на паузі:**
- Consumer перестає забирати пакети з буфера
- Producer блокується, коли всі слоти зайняті
- Пакети не втрачаються

**Producer на паузі:**
- Producer перестає записувати нові пакети
- Consumer обробляє те, що вже є в буфері
- Коли буфер закінчився Consumer блокується

Обидва процеси можуть бути на паузі одночасно

## Зупинка

```bash
kill -INT <pid>    # або Ctrl+C у терміналі процесу
```

Якщо процес заблокований, сигнал обробиться після повернення з системного виклику. Для примусової зупинки: `kill -9 <pid>` (без коректного очищення shared memory).

Якщо shared memory залишилась після аварійного завершення:

```bash
ls /dev/shm/ipc-practice-*
rm /dev/shm/ipc-practice-<pid>
```
