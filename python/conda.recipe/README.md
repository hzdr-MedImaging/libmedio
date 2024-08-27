## Conda build
In order to create conda package:
1. Run: `conda build -c conda-forge conda.recipe`
2. Install locally with `conda install -c conda-forge --use-local pmedio` (optional)
3. Install to local conda channel via `umask 022` followed by e.g:
   `cp ~/conda-bld/linux-64/pmedio-0.2.0-py310h2bc3f7f_0.tar.bz2 /usr/local/petlib/conda-pet/linux-64/`
   and update channel index via: 
   `conda index /usr/local/petlib/conda-pet/`
4. Upload to conda cloud: `anaconda upload conda-bld/linux-64/pmedio-<version>-py310_0.tar.bz2`

## Release new version
1. Make sure that `bumpversion` is installed in your conda env
2. Checkout master branch
3. Run `bumpversion patch` (or `major` or `minor`)
4. Run `git push && git push --tags` (trigger GitHub Actions build) 
5. The rest is going to be made by GitHub (i.e. conda build + upload)
