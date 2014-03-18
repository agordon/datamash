# Compute  [![Build Status](https://travis-ci.org/agordon/compute.png?branch=master)](https://travis-ci.org/agordon/compute)

Command-line Calculations based on input fields.

Website: <http://agordon.github.io/compute/>



## Examples

What's the sum and mean of the values in field 1 ?

    $ seq 10 | compute sum 1 mean 1
    55 5.5

Given a file with three columns (Name, College Major, Score),
what is the average, grouped by college major?

    $ cat scores.txt
    John       Life-Sciences    91
    Dilan      Health-Medicine  84
    Nathaniel  Arts             88
    Antonio    Engineering      56
    Kerris     Business         82
    ...


    # Sort input and group by column 2, calculate average on column 3:

    $ compute --sort --group 2  mean 3 < scores.txt
    Arts             68.9474
    Business         87.3636
    Health-Medicine  90.6154
    Social-Sciences  60.2667
    Life-Sciences    55.3333
    Engineering      66.5385

More Examples: <http://agordon.github.io/compute/examples.html>


## Usage Information

See <http://agordon.github.io/compute/manual.html>


## Download and Installation

Precompiled binaries and installation instructions:
<http://agordon.github.io/compute/download.html>



## Contact

[Assaf Gordon](assafgordon@gmail.com)

<https://github.com/agordon>



## License

Copyright (C) 2014 Assaf Gordon (assafgordon@gmail.com)
GPLv3 or later
