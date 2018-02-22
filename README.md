# Plum

Plum is a [Monte Carlo simulation](https://en.wikipedia.org/wiki/Monte_Carlo_method) package for [polymers](https://en.wikipedia.org/wiki/Polymer). Plum contains many of the **common features** in typical Monte Carlo packges:
* Canonical ensemble Monte Carlo simulations for arbitrary linear polymer-small ion systems.
* Lennard-Jones, electrostatics and bond interactions between atoms/beads.
* Ewald summation for electrostatics calculations.
* Flexible atom/bead parameter settings using bead type and bead partial charge.
* Pivot, crankshaft, replation and center of mass translation Monte Carlo moves for the polymers.

Plum also contains the following **special features**:
* Grand-canonical ensemble simulation with configurational-bias chain insertion and deletion.
* Confined simulations between two infinite surfaces that are periodic along x and y directions.
  * End-grafted polymer brush simulations.
  * Arbitrary distributions of surface interaction sites, charged and/or Lennard-Jones sites.
  * Uniform surface Lennard-Jones potential.
* Osmotic pressure calculations for both bulk and confined systems.

In the future, we would also like to implement the following features in Plum:
* The routines to calculate angle and dihedral angle energies and forces.
* Configurational-bias MC chain moves.
* The routines to simulate arbitrarily branched polymers.

## Getting Started

The following instructions explain how to compile Plum on your local machine. [Example Plum simulations](examples) are also provided to help you get started with your own project using Plum. Detailed explanations of the input and output files, and the Plum source codes can be found in our [wiki](https://github.com/nuwapi/Plum/wiki).

Note that this guide is written for the Linux working environment, but it should also apply to the Mac OS terminal and Xcode.

### Prerequisites

Plum uses free C++ library Eigen and requires Eigen during compilation. Before compiling Plum, [download Eigen](https://eigen.tuxfamily.org/) and decompress it to your local directory. Currently, Plum has only been tested with Eigen versions 3.3.3 and 3.2.8, please let me know if you run into compilation errors when using other versions of Eigen.

You should also make sure that `cmake` and `gcc` are installed properly on your machine.

### Installation

Download the entire [src](src) directory to your local path `your_path_to_plum/plum` and make a `bin` directory under it.

```
cd your_path_to_plum/plum
mkdir bin
```

Find the Makefile under `your_path_to_plum/plum/src` and change the following line in the Makefile
```
INC=-I /home/nuowang/bin/eigen-3.2.8
```

to

```
INC=-I your_path_to_eigen/eigen_x.x.x
```

"x.x.x" represents the version of the Eigen library that you are using. Now, simply `make` to generate the Plum binary under `your_path_to_plum/plum/bin`.

```
cd your_path_to_plum/plum/src
make
```

### Runing examples

You will find 4 example Plum simulations in the [examples](examples) directory. You can `cd` to each directory and run the example by

```
your_path_to_plum/bin/plum < run.in > run.log 
```

Every example directory contains all of the inputs needed to run that example and all of the outputs that will be generated by Plum. If you run an example in its original directory, the provided output files will be overwritten. You could make copies of the provided output files ahead of time if you want to compare your outputs to them. Please further refer to our [input files](https://github.com/nuowang/Plum/wiki/Input-files) wiki page for a detailed explanation of the input parameters and file formats, and please refer to our [output files](https://github.com/nuowang/Plum/wiki/Output-files) wiki page to understand what the Plum outputs mean.

## Contributing

Currently, we have not set up the rules for submitting pull requests. Please directly contact [the developer](https://github.com/nuowang) if you would like to contribute.

## Authors

* **Nuo Wang** - The current developer of Plum, responsible for most of the functionalities in the current version of the code.
* **Rachel Krueger** - The inital framework of the code.

## License

Plum is licensed under the [MIT License](LICENSE.md).

## Acknowledgments

* Dr. Pengfei Zhang for his guidance, suggestions and help in code validation.
* Rachel Krueger for creating the initial framework of what became Plum.
* Dow Chemical Co. for funding the research projects for which Plum was written.
