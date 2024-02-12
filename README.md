# memory-management-sim
Memory Scheduling Simulation

Programın amacı; `scheduler` ve `user_process` ile bellek yönetimi simülasyonu yapmak.

1-) `make` komutunu çalıştırın ya da `scheduler.c` ve `user_process.c` kodlarını aşağıdaki gibi derleyin.

```bash
gcc scheduler.c -o scheduler -lpthread
gcc user_process.c -o user_process
```

2-) `./scheduler` şeklinde scheduler processi çalıştırın.

3-) `./user_process <istemci_ismi> <dosya_ismi>` şeklinde `user_process` processleri (ayrı terminallerde) çalıştırın.

Örnek:

```bash
./user_process ali a.txt
./user_process veli b.txt
```

4-) `user_process` terminallerinde ilgili terminalde, dosya boyutuna bağlı olarak ekranda yazan aralıkta sayfa numarası girerek simülasyonun nasıl çalıştığını görebilirsiniz.

