# Speculative Taint Tracking (STT)

## 1. About STT

Speculative taint tracking (STT) is a hardware defense mechanism for blocking all types of speculative execution attacks in modern processors. All details can be found in our MICRO'19 paper [here](dl.acm.org/citation.cfm?id=3358274). Here is a sample format for citing our work:
```
@inproceedings{yu2019stt,
  title={Speculative Taint Tracking (STT) A Comprehensive Protection for Speculatively Accessed Data},
  author={Yu, Jiyong and Yan, Mengjia and Khyzha, Artem and Morrison, Adam and Torrellas, Josep and Fletcher, Christopher W},
  booktitle={Proceedings of the 52nd Annual IEEE/ACM International Symposium on Microarchitecture},
  pages={954--968},
  year={2019}
}
```

## 2. Implementation

We implement STT using Gem5 simulator. This is built on an early version of Gem5 (commit:38a1e23). To make the simulation close to a commodity processor, we use Gem5's o3 processor. The major changes are:

* add taint tracking logic to track all tainted data
* add delay logic for handling explicit channels (memory instructions)
* add delay logic for handling implicit channels (branch prediction, memory speculation, ld-st forwarding)

## 3. Usage

### 1) Follow the steps for building Gem5 executable.
How to use Gem5 can be found [here](gem5.org).

### 2) We add the following configurations for STT:
* --threat_model [string]: different threat models
    * UnsafeBaseline: unmodified out-of-order processor without protection
    * Spectre: Spectre threat model (covering control-flow speculation)
    * Futuristic: Futuristic threat model (covering all types speculation, exceptions, interrupts)

* --needsTSO [bool]: configure the consistency model
    * True: use Total Store Ordering (TSO) model
    * False: use Relaxed Consistency (RC) model

* --STT [int]: configure STT
    * 0: disable STT (in this case, the defense scheme blocks all speculative transmitters)
    * 1: enable STT

* --implicit_channel [int]: configure implicit channel protection
    * 0: ignore implicit channels
    * 1: enable protection against implicit channels

See https://www.gem5.org/documentation/general_docs/building for more
information on building gem5.

## The Source Tree

The main source tree includes these subdirectories:

* build_opts: pre-made default configurations for gem5
* build_tools: tools used internally by gem5's build process.
* configs: example simulation configuration scripts
* ext: less-common external packages needed to build gem5
* include: include files for use in other programs
* site_scons: modular components of the build system
* src: source code of the gem5 simulator. The C++ source, Python wrappers, and Python standard library are found in this directory.
* system: source for some optional system software for simulated systems
* tests: regression tests
* util: useful utility programs and files

## gem5 Resources

To run full-system simulations, you may need compiled system firmware, kernel
binaries and one or more disk images, depending on gem5's configuration and
what type of workload you're trying to run. Many of these resources can be
obtained from <https://resources.gem5.org>.

More information on gem5 Resources can be found at
<https://www.gem5.org/documentation/general_docs/gem5_resources/>.

## Getting Help, Reporting bugs, and Requesting Features

We provide a variety of channels for users and developers to get help, report
bugs, requests features, or engage in community discussions. Below
are a few of the most common we recommend using.

* **GitHub Discussions**: A GitHub Discussions page. This can be used to start
discussions or ask questions. Available at
<https://github.com/orgs/gem5/discussions>.
* **GitHub Issues**: A GitHub Issues page for reporting bugs or requesting
features. Available at <https://github.com/gem5/gem5/issues>.
* **Jira Issue Tracker**: A Jira Issue Tracker for reporting bugs or requesting
features. Available at <https://gem5.atlassian.net/>.
* **Slack**: A Slack server with a variety of channels for the gem5 community
to engage in a variety of discussions. Please visit
<https://www.gem5.org/join-slack> to join.
* **gem5-users@gem5.org**: A mailing list for users of gem5 to ask questions
or start discussions. To join the mailing list please visit
<https://www.gem5.org/mailing_lists>.
* **gem5-dev@gem5.org**: A mailing list for developers of gem5 to ask questions
or start discussions. To join the mailing list please visit
<https://www.gem5.org/mailing_lists>.

## Contributing to gem5

We hope you enjoy using gem5. When appropriate we advise sharing your
contributions to the project. <https://www.gem5.org/contributing> can help you
get started. Additional information can be found in the CONTRIBUTING.md file.
