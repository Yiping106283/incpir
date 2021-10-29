# Incremental Offline/Online PIR

This repository contains the implementation for our paper "Incremental Offline/Online PIR", which will appear at USENIX Security 2022.

## Code organization

- Implementation for incremental PIR protocol (folder `incremental-pir`)
- Implementation for original CK PIR protocol (folder  `baselines/ck-pir`)
- DPF-PIR baseline imported from C++ DPF-PIR library (folder `baselines/dpf-pir`) [https://github.com/dkales/dpf-cpp]
- End-to-end implementation for latency/throughput testing (folder `netbench` and `torsim`)


## Setup
Please refer to [install.md](./install.md) for installing related dependencies.

## Running experiments
We did experiments on CloudLab, but it can also be run locally on a linux machine.

#### Microbenchmarks

In folder `incremental-pir`, run the script `run.sh`. It will produce each column in the table. See [here](./incremental-pir/readme.md) for more details.


#### Throughput and latency 

For end-to-end tests, run `./offline_server_eval` and `./offline_client_eval` on two machines with the following parameters:

```
-i [ip addr] 
-t [fixed time (0.01 per unit)] -l [offer load] 
-d [db size] -s [set size] -n [nbrsets] 
-a [incprep] -b [baseline]
```

where parameters for the server and the client should be the same except ip.
See [incremental pir](./netbench/readme.md), [ck baseline](./baselines/ck-pir/readme.md) and [dpf baseline](./baselines/dpf-pir/readme.md) for more details.

The server program needs to be manually killed, otherwise, it will always be waiting for new requests.

#### Communication measured in protobuf

See [incremental pir](./netbench/readme.md), [ck baseline](./baselines/ck-pir/readme.md) and [dpf baseline](./baselines/dpf-pir/readme.md) in each folder.

#### Tor trace simulation

In folder `torsim`, run `sh run.sh`.

## Misc

We have updated the code slightly from the version in [Artifact Evaluation](https://github.com/Yiping106283/incpir/tree/main).