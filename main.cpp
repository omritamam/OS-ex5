#include <iostream>
#include <unistd.h>
#include <stdio.h>

struct ContainerInfo {
    char* path;
    char* hostname;
    size_t len_hostname;
    int max_processes;
};

int InitContainer(void* function, void* containerInfo) {
    auto* info = (ContainerInfo*) containerInfo;
    return 0;
    // change root
    chroot(info->path);
    //change hostname
    sethostname(info->hostname, info->len_hostname);

    // create cgrop file
    // mkdir -p /sys/fs/cgroup/pids
    // echo max_processes > /sys/fs/cgroup/pids/parent/pids.max

    // mount to the container
    // mount -t cgroup -o pids none /sys/fs/cgroup/pids

    // 3 Change the working directory into the new root directory
    chdir(info->path);

    //4. Mount the new procfs
    // mount -t proc none /proc
    wait( exec() );

}

int newContainer(void* function, void* stack_pointer) {
    clone(InitContainer,stack_pointer, CLONE_NEWNS | CLONE_NEWPID
    return 0;
}

int main(){
    ContainerInfo info{};
    info.path = "/home/user/container";
    info.hostname = "container";
    info.len_hostname = strlen(info.hostname);
    info.max_processes = 10;
    newContainer(NULL, &info);
    return 0;
}
