# catbin

(net)cat whatever you want to upload!

Inspired by [solusipse/fiche](https://github.com/solusipse/fiche), but does not
have the requirement of regular text files (binary ones supported too).
There is no exact file size limit (I have successfully uploaded a 2GiB file over the internet),
but I would recommend finding a more purpose-built tool for larger files.

## Performance
catbin uses evented I/O through libuv, so hopefully it is pretty fast and
resistant to some attacks.

I don't personally do any optimizations at all.

## Example usage (client)

```
$ cat upload.zip | nc example.com 7777 -q 0
http://example.com/sd2e
```

```
$ # note that the -H 'Expect:' is not necessary but is preferred.
$ curl http://example.com:7777/ -H 'Expect:' --upload-file upload.zip 
http://example.com/sd2e
```

## Example usage (server)

To build `catbin`:
```
$ # install libuv development headers+library (e.g. libuv1-dev on debian)
$ git clone https://github.com/ohnx/catbin
...
$ make
...
```

To run `catbin`:
```
$ ./catbin -d "https://example.com/"
```

Some form of web server must also be provided. catbin stores uploaded data as files in the directory.
Simply serving the files should suffice. Pair it with [nginx-guess-mime](https://github.com/ohnx/nginx-guess-mime)
to return the correct MIME types for the files, too!
