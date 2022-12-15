# 2D Graphics

A 2D Graphics Engine that implements geometric primitives, scan conversion, clipping, transformations, compositing, image sampling, gradients, antialiasing, filtering, parametric curves, and geometric stroking.

## Testing

### Unit Tests

To run/test your code, a few unit tests are supplied:

```shell
$ make
$ ./image -e expected
$ ./tests
$ ./bench
```

For more details when testing:

```shell
$ make dbench
$ ./dbench
$
$ mkdir diff
$ ./image -e expected -d diff
$ open diff/index.html
$
$ ./tests -v
$ ./bench -m substring-to-match
```

### Images

Assignments also have a set of images that utilize the implemented features.

```shell
$ make image
$ ./image -e expected -v
```


## License

This project is licensed under the MIT License. See the [`LICENSE`](LICENSE) file for more information.


## Authors

Daniel Usim (danielusim@gmail.com)
