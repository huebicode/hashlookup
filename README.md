# Hash Lookup
### Multithreaded and hardware accelerated file hashing application with YARA integration
![HashLookupv1 2](https://github.com/huebicode/hashlookup/assets/3885373/4e25adcd-d519-4449-b759-13352fe627cf)

- hashing algorithms: MD5, SHA1 and SHA256
- scan files with YARA rules
- fast file hashing through multithreading
- hardware accelerated sha-hashing (processor with "Intel SHA extensions" support needed)
- color and filter out doubles
- show file size, file extension, MIME type, file type, dirpath and fullpath
- export to clipboard or .tsv
- zip selected or all files (up to 4GB per file)

### Dependencies and Resources

- Qt Framework v6.5 (LGPLv3)
- OpenSSL v3.1.0 (Apache License 2.0)
- yara v4.3.0 (BSD-3-Clause License)
- libmagic v5.4
- zlib v1.213 (zlib License)
- QuaZip v1.4 (LGPLv2.1)
- HeaderSortingAdapter (MIT License)
- Roboto Font (Apache License 2.0)
