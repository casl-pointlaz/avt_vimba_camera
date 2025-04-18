# PointLaz RailCam 
The PointLaz RailCam Camera Software is designed to perform 3D measurements of elevator rails
as part of the Lazaruss scanner.

[![Commitizen friendly](https://img.shields.io/badge/commitizen-friendly-brightgreen.svg?style=flat-square)](http://commitizen.github.io/cz-cli/)
[![Conventional Commits](https://img.shields.io/badge/Conventional%20Commits-1.0.0-yellow.svg?style=flat-square)](https://conventionalcommits.org)

## Test
```bash
mkdir build && cd build
cmake -DRAILCAM_BUILD_TESTS=ON .. && make -j $(nproc)
ctest .
```

## Develop
To use automatic version bump, use [conventional commits](https://conventionalcommits.org)
and install [commitizen](http://commitizen.github.io/cz-cli/) with `pip install -U commitizen`. To bump a version,
use `cz bump`.