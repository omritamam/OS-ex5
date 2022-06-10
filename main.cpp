#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <a.out.h>
#include <csignal>
#include <sys/wait.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/resource.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/echo.h>
#include<memory>
#include <fstream>

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
    // change root
    auto rt = chroot(info->rpath);
    if (rt != 0) {
        perror("chroot");
        return -1;
    }
    //change hostname
    rt = sethostname(info->hostname, info->len_hostname);
    if (rt != 0) {
        perror("sethostname");
        return -1;
    }

    // create cgrop file
    mkdir("/sys/fs/cgroup/pids", 0755);
    ofstream outfile;
    outfile.open("/sys/fs/cgroup/pids/cgroup.procs");
    outfile << to_string(getpid());
    outfile.close();

    ofstream cgroup("/sys/fs/cgroup/pids/pids.max");
    cgroup << to_string(info->max_processes);
    cgroup.close();

    //notify to release
    ofstream notify("/sys/fs/cgroup/pids/notify_on_release");
    notify << "1";
    notify.close();

    /**
     *        int mount(const char *source, const char *target,
                 const char *filesystemtype, unsigned long mountflags,
                 const void *data);
     */
     //TODO: Check the arguments
    rt = mount("proc", "/proc", "proc", 0, 0);
    if (rt != 0) {
        perror("mount");
        return -1;
    }

    // 3 Change the working directory into the new root directory
    rt = chdir(info->rpath);
    if (rt != 0) {
        perror("chdir");
        return -1;
    }

    //4. Mount the new procfs
    // mount -t proc none /proc
    rt =  execvp((char *)info->functionPathInContainer, (char* const*) info->args);
    if (rt == -1) {
        perror("execvp");
        return -1;
    }
    //wait( int execvp(const char *file, char *const argv[]) );
    return 0;
}

int newContainer(ContainerInfo* containerInfo) {
    auto * stack_pointer = malloc(8192);//TODO macro for stack size
    auto * stack_pointer_top = (char *)stack_pointer + 8192;
    return clone(InitContainer,stack_pointer_top, CLONE_NEWNS | CLONE_NEWPID | CLONE_NEWUTS | SIGCHLD, containerInfo);
}


void proc_exit(int i)
{
    auto rt = umount("/proc");
    if (rt != 0) {
        perror("umount");
        exit(1);
    }

//    int wstat;
//    union wait wstat;
//    pid_t	pid;
//
//    while (true) {
//        pid = wait3 (&wstat, WNOHANG, (struct rusage *)NULL );
//        if (pid == 0)
//            return;
//        else if (pid == -1)
//            return;
//        else
//            printf ("Return code: %d\n", wstat.w_retcode);
//    }
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
    signal (SIGCHLD,proc_exit);

    auto pidChild = newContainer(&info);
    if (pidChild == -1) {
        std::cout << "Error creating container" << std::endl;
        return -1;
    }
    return 0;
}
