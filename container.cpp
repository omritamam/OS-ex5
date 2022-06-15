#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <csignal>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include<memory>
#include <fstream>
//#include <boost/filesystem.hpp>
//...
using namespace std;
struct ContainerInfo {

    char* rpath;
    char* hostname;
    size_t len_hostname;
    int max_processes;
    void* functionPathInContainer;
    void* args;
};
// rundeb10 -cow /cs/course/current/os/ex5/ex5-deb10.qcow2 -bind cs/usr/omri.tamam/<PATH_TO_PROJECT_DIR>/ -serial -root -snapshot -- -net user,hostfwd=tcp:127.0.0.1:2222-:22 -net nic,model=virtio
// Password to root is “toor”
int InitContainer(void* containerInfo) {
    auto* info = (ContainerInfo*) containerInfo;

    //1. change hostname
    auto rt = sethostname(info->hostname, info->len_hostname);
    if (rt != 0) {
        printf("sethostname");
        fflush(stdout);
        return -1;
    }
    printf("hostname changed\n");
    fflush(stdout);


    // 1. change root
    rt = chroot(info->rpath);
    if (rt != 0) {
        printf("chroot fails\n");
        fflush(stdout);
        return -1;
    }
    printf("%s\n", "chroot done");
    fflush(stdout);

    // 3. Change the working directory into the new root directory
    rt = chdir(info->rpath);
    if (rt != 0) {
        printf("chdir");
        fflush(stdout);
        return -1;
    }
    printf("finish chdir\n");
    fflush(stdout);


    // 2. create cgrop file
    rt = system("mkdir -p /sys/fs/cgroup/pids");
    if (rt != 0) {
        printf("mkdir fails\n");
        fflush(stdout);

        return -1;
    }
    printf("%s\n", "mkdir done!!");
    fflush(stdout);

    ofstream outfile;
    outfile.open("/sys/fs/cgroup/pids/cgroup.procs");
    outfile << to_string(getpid());
    outfile.close();

    outfile.open("/sys/fs/cgroup/pids/pids.max");
    outfile << to_string(info->max_processes);
    outfile.close();

    //2. notify to release
    outfile.open("/sys/fs/cgroup/pids/notify_on_release");
    outfile << "1";
    outfile.close();
    printf("%s\n", "create cgrop file done");
    fflush(stdout);


    //4. Mount the new procfs
    rt = mount("proc", "/proc", "proc", 0, nullptr);
    if (rt != 0) {
        printf("mount");
        fflush(stdout);
        return -1;
    }
    printf("finish mount\n");
    printf("-----------------------------------------------------\n");
    fflush(stdout);
    // 5. Run the terminal/new program
    rt =  execv((char *)info->functionPathInContainer, (char* const*) info->args);
    if (rt == -1) {
        printf("execvp error\n");
        fflush(stdout);

        return -1;
    }
    return 0;
}

int newContainer(ContainerInfo* containerInfo) {
    auto * stack_pointer = malloc(8192);//TODO macro for stack size
    auto * stack_pointer_top = (char *)stack_pointer + 8192;
    return clone(InitContainer,stack_pointer_top, CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWUTS | SIGCHLD, containerInfo);
}


void proc_exit(int i)
{
    //change to full path
    auto rt = umount("/proc");
    if (rt != 0) {
        printf("umount fails\n");
        exit(1);
    }
}

// usage: ./container <new_hostname> <new_filesystem_directory> <num_processes> <path_to_program_to_run_within_container> <args_for_program>
int main(int argc, char* argv[]) {
    ContainerInfo info{};
    info.rpath = argv[2];
    info.hostname = argv[1];
    info.len_hostname = strlen(info.hostname);
    info.max_processes = std::stoi(argv[3]);
    info.functionPathInContainer = argv[4];
    char* _args[] ={(char *)info.functionPathInContainer, (char *)argv + 5, (char *)0};
    info.args = _args;
//    signal (SIGCHLD,proc_exit);

    auto pidChild = newContainer(&info);
    if (pidChild == -1) {
        std::cout << "Error creating container" << std::endl;
        return -1;
    }
    return 0;
}
/**
* 1. Run the command in the terminal:

rundeb10 -cow /cs/course/current/os/ex5/ex5-deb10.qcow2 -bind /cs/usr/omri.tamam/CLionProjects/OS-ex5/ -root -snapshot -- -net user,hostfwd=tcp:127.0.0.1:2222-:22 -net nic,model=virtio

The VM shpuld open

2. Notice that the file that is given to -bind is recognized by the VM and whnever you are making changes on the computer
it will automatically be updated.

3. Open the terminal in the VM
4. run the command: su root. Set the password to 'toor'
5. Navigate to 'root@debian10'
6. cd /cs/usr/omri.tamama/CLionProject/OS-ex5
Now you are in your user and you can navigate to the folder you gave as an argument.
7. Compile you code file – g++ -std=c++11 -Wall container.cpp -o container
8. Run your code file g++ - ./container host1 filesystem 3 “/bin/bash”

Good Luc
*/