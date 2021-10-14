# Artifact Evaluation

This repository contains the implementation for our paper "Incremental Offline/Online PIR", which will appear at USENIX Security 2022.

## Code organization

- Implementation for incremental PIR protocol (folder `incremental-pir`)
- Implementation for original CK PIR protocol (folder  `baselines/ck-pir`)
- DPF-PIR baseline imported from C++ DPF-PIR library (folder `baselines/dpf-pir`) [https://github.com/dkales/dpf-cpp]
- End-to-end implementation for latency/throughput testing (folder `netbench` and `torsim`)


## Setup
Run `sudo bash install.sh` to install all the dependencies (openssl, protobuf, python3, matplotlib). 


## Running experiments
We did experiments on CloudLab, but it can also be run locally on a linux machine.

#### Microbenchmark (Figure 7)

In folder `incremental-pir`, run the script `run.sh`. It will produce each column in the table.


#### Throughput and latency (Figure 8a and 8b)

These are figures for the online phase (8a) and offline (preprocessing or hint updates) phase (8b).

In folder `netbench/figs`, run the script `run.sh`. It includes the metadata we measured and you will get figures in `netbench/figs`. 


To test functionality for incremental CK, run `./offline_server_eval` and `./offline_client_eval` on two machines with the following parameters:

```
-i [ip addr] 
-t [fixed time (0.01 per unit)] -l [offer load] 
-d [db size] -s [set size] -n [nbrsets] 
-a [incprep] -b [baseline]
```

where parameters for the server and the client should be the same except ip. For example,

Prep

```sh
./offline_server_eval -i 0.0.0.0:6666 -t 1000 -l 50 -d 7000 -s 84 -n 1020 -b
./offline_client_eval -i [XXX.XXX.XXX.XXX]:6666 -t 1000 -l 50 -d 7000 -s 84 -n 1020 -b
```

IncPrep

```sh
./offline_server_eval -i 0.0.0.0:6666 -t 1000 -l 50 -d 7000 -s 84 -n 1020  -a 70
./offline_client_eval -i [XXX.XXX.XXX.XXX]:6666 -t 1000 -l 50 -d 7000 -s 84 -n 1020 -a 70
```

The server will print process time for each task and the client will print latency for each task. 


To test CK, in folder `baselines/CK` , run 

```
./server -o -i 0.0.0.0:6666 -t 10 -g 13 -d 7000
./online_client_eval -i [XXX.XXX.XXX.XXX]:6666 -t 10 -l 100 -g 13 -d 7000
```


To test DPF-PIR, in folder `baselines/dpf-pir`, run

```
./server -t 10 -l 15 -i 0.0.0.0:6666 -g 13
./client -t 10 -l 15 -i [XXX.XXX.XXX.XXX]:6666 -g 13
```

(The server program needs to be manually killed, otherwise, it will always be waiting for new requests)

#### Communication in protobuf (Figure 9a and 9b)

These are figures for communication in online and offline (preprocessing or hint updates) phases.

In folder `netbench/figs`, run the script `run.sh`. See figures in `netbench/figs`. 


#### Tor trace simulation (Figure 10a and 10b)

These are figures for server computation cost (10a) and client storage growth (10b).

In folder `torsim`, run `sh run.sh` and see `Figure-12a.pdf` and `Figure-12b.pdf` in this folder.
