# catbin

(net)cat whatever you want to upload!

## Example usage (client)

```
$ cat upload.zip | nc example.com 9674
http://example.com/sd2e
```

## Example usage (server)

To run `catbin`:
```
$ git clone https://github.com/ohnx/catbin
...
$ make
...
$ ./catbin
```

Some form of web server must also be provided. Catbin stores uploaded data as files in the directory.
Simply serving the files should suffice.

