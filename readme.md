Lockd - because all the good names were taken

The aim of this project is to limit access to sensitive files by normal user processes.
The main motivation being to protect browser sessions/cookies from session hijacks.

How it works:

We start by creating a block device file. With 0700 permissions, only root can read the contents.
Inside this we create an ext4 filesystem owned by the target user.

Then when we want to run an application, we start by having root create a new mount namespace.
Inside that we mount the block device into the user's home directory. For example, ~/.mozilla/firefox
And then we drop down to a user process, still in that namespace, and start the target application

As the namespace is root-owned it cannot be entered by a non-root process.
And as the namespace has its own mounts, the contents of our ext4 filesystem is not accessible by outside processes.

Block device: unreadable (0700)
Namespace: inaccessible (root-controlled)
Filesystem: invisible (isolated namespace)


The project uses standard tools that should be available in all distributions, but does assume `sudo` and bash are present.
Otherwise, coreutils and linux-utils are all that's required.
(This does not mean it will work for all distributions.)


Project layout:

Currently the project is split into two main files:
`src/lockd` - this handles the CLI, and creating the namespace.
`src/run` - this handles mounting and running the application.



Installation:

`sudo ./install install`

Removal:

`sudo ./install uninstall`


How to use it:
Right now the block device needs to be manually created
`See docs/making a store` for an example

A profile also needs to be written.
This includes the name of the store, the mount-point and the command to run

If properly configured, simply run:
`lockd run <profile>`


Issues and limits:
This code currently relies on having a fixed-size file to store the filesystem.
This limits its use to situations where a maximum size is known ahead-of-time.

Contributions:
Yes please! I'm open to discussions and improvements, though hopefully not too much feature creep.

I'm a novice in most of the tech used here so I'm sure there's plenty to improve.
That includes markdown, so if anyone can clean it up then that would be greatly appreciated!

Contact:

Discord: @whosy
Telegram: @whosy
Matrix: @whosy:matrix.org


Also find me in the Linux Mint Discord or Matrix groups.
