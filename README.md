MetaMDBG is a lightweight assembler for long and accurate metagenomics reads.

Developper: Gaëtan Benoit  
Contact: gaetanbenoitdev at gmail dot com

## Installation

### Conda

Choose a directory to install metaMDBG, then copy-paste all the following commands.
This will create a conda environment, named metaMDBG, with all dependencies installed.
After successful installation, an executable named metaMDBG will appear in ./build/bin.

```
git clone https://github.com/GaetanBenoitDev/metaMDBG.git
cd metaMDBG
conda env create -f conda_env.yml
conda activate metaMDBG
conda env config vars set CPATH=${CONDA_PREFIX}/include:${CPATH}
conda deactivate
conda activate metaMDBG
mkdir build
cd build
cmake ..
make -j 3
```

### Building from source

**Prerequisites**
- gcc 9.4+
- cmake 3.10+
- zlib
- openmp
- minimap2 2.24+
- wfmash
- samtools 1.6+ (using htslib)

```
git clone https://github.com/GaetanBenoitDev/metaMDBG.git
cd metaMDBG
mkdir build
cd build
cmake ..
make -j 3
```

## Usage

```
./metaMDBG asm outputDir reads... {OPTIONS}

	outputDir     Output dir for contigs and temporary files
	reads...      Read filename(s) (separated by space)
	-t            Number of cores [3]
```

MetaMDBG will generate polished contigs in outputDir ("contigs.fasta.gz").

## Advanced usage
 
```
# Set minimizer length to 16 and use only 0.2% of total k-mers for assembly.
./metaMDBG asm ./outputDir reads.fastq.gz -k 16 -d 0.002

# Stop assembly when reaching a k-mer length of 5000 bps.
./metaMDBG asm ./outputDir reads.fastq.gz -m 5000
```

## Generating an assembly graph

Assembly graph (.gfa) can be generated after a successful run of metaMDBG.
First, display the available k-min-mer size and their corresponding sequence length in bps.
```
./metaMDBG gfa ./outputDir 0
```
Note that lower k values will produce graph with high connectivity but shorter unitigs, while higher k graphs will be more fragmented but with longer unitigs.

Then, choose a k value and produce the graph.
```
./metaMDBG gfa ./outputDir 21
```

Note that sequence in the gfa are not polished, they will have the same error rate as in the original reads.

## License

metaMDBG is freely available under the [MIT License](https://opensource.org/license/mit-0/).

## Citation

