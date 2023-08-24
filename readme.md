#BNNQuanalyst

This is the repository of the BNN quantitative verifier "BNNQuanalyst", which is introduced in paper "Precise Quantitative Analysis of Binarized Neural Networks: A BDD-based Approach".

In this paper, we make the following main contributions:
- We introduce a novel algorithmic approach for encoding BNNs into BDDs that exactly preserves the semantics of BNNs, which supports incremental encoding;
- We explore parallelization strategies at various levels to accelerate BDD encoding, most of which can significantly improve the BDD encoding efficiency;
- We propose a framework for quantitative verification of BNNs and in particular, we demonstrate the robustness analysis and interpretability of BNNs;
- We implement the framework as an end-to-end tool BNNQuanalyst, and conduct thorough experiments on various BNNs, demonstrating its efficiency and effectiveness. 

## Quick Start

### Dependencies
BNNQuanlyast uses [Sylvan](https://github.com/utwente-fmt/sylvan) as one of the BDD packages, and `GMP` library (e.g., `libgmp-dev`)
 is required.

### Build the tool

```
mkdir build
cmake CMakeList.txt -B build
cd build; make
```
Then, BNNQuanalyst can be found under the project source folder.

## Usage
You can print the help information by option `-h`: 
```
./BNNQuanalyst -h
```
Options and arguments: `./BNNQuanalyst [option] [arg]`
```
-h: print help information (action mode)
-v: verify the input query (action mode)
-q arg: specify the query_file to verify (arg=query_file_name)
-IP arg: whether use input propagation strategy or not (arg=1: Yes; arg=0: No)
-DC arg: whether use divide-and-conquer Strategy or not (arg=1: Yes; arg=0: No)
-e arg: specify the engine used (args=cudd or sylvan or cudd-npaq or npaq-cudd or npaq-sylvan)
-l arg: specify the level of parallelization strategies (arg = 1,2,3) (only for Sylvan package)
-th arg: specify the number of workers used for Sylvan (arg>=1) (only for Sylvan package)
```
BNN Models, inputs and query files to verify must be put in the folder "./benchmarks", of which the structure is: 
```
├── models           
│   ├── blk1         parameter_csv for the first internal block (bn_bias, bn_mean, bn_var, bn_weight, lin_bias, lin_weight)
│   ├── ...(blki)    parameter_csv for the i-th internal block
│   └── out_blk      parameter_csv for the output block
├── queries          query_file to be verified (format can be check in the given demo files, currently we support four types of queries: target_robustness, compute_SD, PI_explain and EF_explain)
└── inputs          
    ├── input_instance_file       (the input instance of interest)
    ├── fixed_set_file       (the fixed indices which defines the input region)
    └── target_class_file      (the output classes for which we construct the BDD, e.g., any subset of {0,1,...,9} for 10 class classfification tasks)
```

### Example
- Verify the queries written in the file "./benchmarks/queries/query_file_demo" using `CUDD` package:
  
  `./BNNQuanalyst -v -q query_file_demo -IP 1 -DC 1 -e cudd`
- Verify the queries written in the same file using 'Sylvan' package with 10 threads under Level 2 (L2) parallelization strategy:

  `./BNNQuanalyst -v -q query_file_demo -IP 1 -DC 1 -e sylvan -l 2 -th 10`

Note that, for the PI_explain and EF_explain queries, BNNQuanalyst will output the analysis results into folders "./outputs/PI_explain" and "./outputs/EF_explain" respectively.

BNNQuanalyst also implements three another engine "cudd-npaq", "npaq-cudd" and "npaq-sylvan" for the comparison (target_robustness query type only):
- cudd-npaq: BNNQuanalyst encodes each verification task into a CNF file, and users can use other model counting tools (e.g., Approxmc) on it. Such CNF files are exported to folder "./outputs/CNFSets_CUDD"";
- npaq-cudd: BNNQuanalyst first reads cnf files from folder "./benchmarks/CNFSets", then encode it into a BDD, and finally count the minterm (using `CUDD` package);
- npaq-sylvan: same as npaq-cudd, except that it uses `Sylvan` package.

User can also explicitly set different sizes of the nodes table and the operation cache for `Sylvan` usage via function `sylvan_set_sizes()`.

## Publications

If you use BNNQuanalyst, please kindly cite our papers:

- Zhang, Y., Zhao, Z., Chen, G., Song, F., Chen, T.: BDD4BNN: A BDD-based quantitative analysis framework for binarized neural networks. In: Proceedings of the 33rd International Conference on Computer Aided Verification, 2021.
- Zhang, Y., Zhao, Z., Chen, G., Song, F., Chen, T.: Precise quantitative analysis of binarized neural networks: A bdd-based approach. ACM Transactions on Software Engineering and Methodology, 2023.



