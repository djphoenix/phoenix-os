### Source download
* [GIT](https://en.wikipedia.org/wiki/Git_(software)): `git clone https://git.phoenix.dj/phoenix/phoenix-os/`
* [ZIP file](https://git.phoenix.dj/phoenix/phoenix-os/repository/archive.zip) from master branch.

### Build prerequisites
* [CLANG](https://clang.llvm.org/) 5 or later
* [GNU make](http://www.gnu.org/software/make/)

### Build on GNU/Linux
```bash
# Install GCC, MAKE, GIT
sudo apt-get install clang-5 lld-5 make git
# Get source code
git clone https://git.phoenix.dj/phoenix/phoenix-os.git
# Build
cd phoenix-os
make all
```
### Build on Mac OS X

1. Install Xcode (from App Store), launch (at least once) and install Command Line Tools
2. Open Terminal.app (on Application/Utilities) and execute commands:

```bash
# Install homebrew (if not installed)
/usr/bin/ruby -e "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"
# Install GIT, GIT-LFS, MAKE, CLANG 5
brew install git git-lfs make llvm
# Get source code
git clone https://git.phoenix.dj/phoenix/phoenix-os.git
# Build
cd phoenix-os
make all
```

### Build on Microsoft® Windows®
#### Install Cygwin

1. Download Cygwin installer that match your OS
    * [32-bit](http://cygwin.com/setup-x86.exe)
    * [64-bit](http://cygwin.com/setup-x86_64.exe)
2. Launch downloaded file
3. Click "Next" until you reach "Choose A Download Site" page.
4. On "Choose A Download Site" page select preferred mirror to download components. If doubt, select first of available.
5. On "Select Packages" page select at least these components:
    * Devel/clang
    * Devel/git
    * Devel/make
6. On "Resolving Dependencies" verify that "Select required packages" is checked, click "Next".
7. Wait for installation completion.

#### Fetch source code of PhoeniX OS

##### Download from Git
1. Launch `Cygwin/bin/bash.exe`
2. If you was not included Cygwin in your PATH environment option, execute `export PATH=/usr/bin:/bin`
3. Change your working directory to drive C: `cd /cygdrive/c/`
4. For go into subfolders: `cd NAME`
5. Execute `git clone https://git.phoenix.dj/phoenix/phoenix-os.git`

##### Fetch ZIP archive of `master` branch
1. Download and extract [archive](https://git.phoenix.dj/phoenix/phoenix-os/repository/archive.zip)
2. Launch `Cygwin/bin/bash.exe`
3. If you was not included Cygwin in your PATH environment option, execute `export PATH=/usr/bin:/bin`
4. Change your working directory to folder with extracted files
		* Example: Go to Drive C: `cd /cygdrive/c/`
		* For go into subfolders: `cd NAME`

#### Build
Execute `make all` in Cygwin terminal (that still open from "Fetch source" step). Wait for execution completion.
