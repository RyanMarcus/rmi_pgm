A repoistory to test PGMs and RMIs on different platforms using a much simpler benchmark harness than SOSD.

The only dependencies are:

* A C++ compiler with support for C++ 17
* `xz` compression tool (`pacman -S xz`)
* `zstd` compression tool (`pacman -S zstd`)
* `wget` to download files (`pacman -S wget`)

You can execute the benchmark with:
```bash
make results.txt
```

... which should download the data, uncompress the RMIs, and record the benchmark data and your current CPU into the file results.txt.


If you want to reproduce the RMI parameters used, here are the commands to use on the RMI learner:
```
cargo run --release -- ../data/wiki_ts_200M_uint64 wiki cubic,linear 2097152 -d rmi_data/ -e
cargo run --release -- ../data/fb_200M_uint64 fb robust_linear,linear 2097152 -d rmi_data/ -e
cargo run --release -- ../data/books_200M_uint64 books linear_spline,linear 2097152 -d rmi_data/ -e
cargo run --release -- ../data/osm_cellids_200M_uint64 osm cubic,linear 2097152 -d rmi_data/ -e
```
