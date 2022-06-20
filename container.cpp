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
    char *rpath;
    char *hostname;
    size_t len_hostname;
    int max_processes;
    void *functionPathInContainer;
    void *args;
    void *stack_pointer;

    char **originalArgv;
    int originalArgc;
};

int changeHostname(ContainerInfo *info);

int changeRoot(ContainerInfo *info);

int changeDirectory(ContainerInfo *info);

int createCgroups(ContainerInfo *info);

int mountFiles();

int runProcess(ContainerInfo *info);

// rundeb10 -cow /cs/course/current/os/ex5/ex5-deb10.qcow2 -bind /cs/usr/omri.tamam/CLionProjects/OS-ex5 -serial -root -snapshot -- -net user,hostfwd=tcp:127.0.0.1:2222-:22 -net nic,model=virtio
// Password to root is “toor”

int removeFiles(ContainerInfo info);

int unmountFiles(ContainerInfo info);

int InitContainer(void *containerInfo) {
    auto *info = (ContainerInfo *) containerInfo;
//    printf("  inside container  ");
//    system("ps");
    // 3. Change the working directory into the new root directory
    changeHostname(info);
    changeRoot(info);
    changeDirectory(info);
    //4. Mount the new procfs
    mountFiles();

    // 4. create cgrop file
    createCgroups(info);

    // 5. Run the terminal/new program
    runProcess(info);
    return 0;

}

//------------------------------------------------------------------------------------------//

int runProcess(ContainerInfo *info) {
//    auto argsToCommandLen = (info->originalArgc) - 5;
//    char** args_func = new(std::nothrow) char*[argsToCommandLen + 2];
//    for (int i =1; i<argsToCommandLen; i++) {
//         args_func[i] = (info->originalArgv)[4+i];
//        }
//    args_func[argsToCommandLen] = nullptr;
////    args_func[argsToCommandLen+1] = nullptr;
//for(int i= 0; i<argsToCommandLen + 1 ; i++){
//    printf("%s", args_func[i]);
//    fflush(stdout);
//}
////auto rt = execvp((char *)info->functionPathInContainer, args_func);
//



    auto rt = execvp((char *) info->functionPathInContainer, (char *const *) info->args);
    if (rt == -1) {
        printf(" execvp error num %d\n", errno);
        fflush(stdout);
        return -1;
    }
    return 0;
}

int mountFiles() {
    auto rt = mount("proc", "/proc", "proc", 0, 0);
    if (rt != 0) {
        printf("mount error \n");
        fflush(stdout);
        return -1;
    }
    return 0;
}

int createCgroups(ContainerInfo *info) {
    auto rt = system("mkdir -p /sys/fs/cgroup/pids -m 0755");
    if (rt != 0) {
        printf("mkdir fails\n");
        fflush(stdout);

        return -1;
    }
    ofstream outfile;
    outfile.open("/sys/fs/cgroup/pids/cgroup.procs");
    outfile << to_string(getpid());
    outfile.close();
    chmod("/sys/fs/cgroup/pids/cgroup.procs", 0755);

    outfile.open("/sys/fs/cgroup/pids/pids.max");
    outfile << to_string(info->max_processes);
    outfile.close();
    chmod("/sys/fs/cgroup/pids/pids.max", 0755);

    outfile.open("/sys/fs/cgroup/pids/notify_on_release");
    outfile << "1";
    outfile.close();
    chmod("/sys/fs/cgroup/pids/notify_on_release", 0755);


    return 0;

}

int changeDirectory(ContainerInfo *info) {
    auto rt = chdir("/");
    if (rt != 0) {
        printf("  chdir fails error num is %d , path is %s\n", errno, info->rpath);
        fflush(stdout);
        return -1;
    }
    return 0;

}

int changeRoot(ContainerInfo *info) {
    auto rt = chroot(info->rpath);
    if (rt != 0) {
        printf("chroot fails, error num %d\n", errno);
        fflush(stdout);
        return -1;
    }
    return 0;

}

int changeHostname(ContainerInfo *info) {
    char myname[256 + 1];
    gethostname(myname, 256);
//    printf("old Hostname is %s\n", myname);
//    fflush(stdout);

//    printf("Changing hostname to %s\n", info->hostname);
//    fflush(stdout);
    auto rt = sethostname((info->hostname), info->len_hostname);
    if (rt != 0) {
        printf("sethostname errror is %d", errno);
        fflush(stdout);
        return -1;
    }
    gethostname(myname, 256);
//    printf("Hostname is %s\n", myname);
//    fflush(stdout);
    return 0;
}

//------------------------------------------------------------------------------------------//

int newContainer(ContainerInfo *containerInfo) {
    auto *stack_pointer_top = (char *) containerInfo->stack_pointer + 8192;
    return clone(InitContainer, stack_pointer_top, CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWUTS | SIGCHLD, containerInfo);
}

// usage: ./container <new_hostname> <new_filesystem_directory> <num_processes> <path_to_program_to_run_within_container> <args_for_program>
int main(int argc, char *argv[]) {
//    fprintf(stdout, "hello");
//    fflush(stdout);
    ContainerInfo info{};
    info.rpath = argv[2];
    info.hostname = argv[1];
    info.len_hostname = strlen(info.hostname);
    info.max_processes = std::stoi(argv[3]);
    info.functionPathInContainer = argv[4];
    info.originalArgv = argv;
    info.originalArgc = argc;
    char *funcArgs[argc - 3];
    for (int i = 4; i < argc; i++) {
        funcArgs[i - 4] = argv[i];
    }
    funcArgs[argc - 4] = nullptr;

//    char *_args[] = {(char *) info.functionPathInContainer, (char *) argv + 5, (char *) 0};
    info.args = funcArgs;
    auto *stack_pointer = malloc(8192);//TODO macro for stack size
    info.stack_pointer = stack_pointer;


    auto argsToCommandLen = argc - 5;
    char **args_func = new(std::nothrow) char *[argsToCommandLen + 2];
    for (int i = 1; i < argsToCommandLen; i++) {
        args_func[i] = argv[4 + i];
    }
    args_func[argsToCommandLen] = nullptr;
    //    args_func[argsToCommandLen+1] = nullptr;
//    for(int i= 0; i<argsToCommandLen + 1 ; i++){
//        printf("ars is %s", args_func[i]);
//        fflush(stdout);
//    }









    auto pidChild = newContainer(&info);
    if (pidChild == -1) {
        std::cout << "Error creating container" << std::endl;
        return -1;
    }
    auto rt = wait(NULL);
    if(rt < 0){
        printf("wait error");
    }
    unmountFiles(info);
    removeFiles(info);
    free(stack_pointer);
    return 0;
}

int unmountFiles(ContainerInfo info) {
    std::string pathToUnmount = info.rpath;
    pathToUnmount += "/proc";
    auto rt = umount(pathToUnmount.c_str());
    if (rt != 0) {
        int errorNum = errno;
        printf("error is %d\n ", errorNum);
        printf("umount fails\n");
        exit(1);
    }
    return 0;

}

int removeFiles(ContainerInfo info) {
    std::string command = "rm -rf ";
    command += info.rpath;
    command += "/sys/fs";
    system(command.c_str());
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