ARG ROS_DISTRO=lyrical
FROM ros:${ROS_DISTRO}-ros-base

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get -q update \
    && apt-get -q -y upgrade \
    && apt-get -q install --no-install-recommends -y \
    git \
    sudo \
    wget \
    curl \
    python3-vcstool \
    && apt-get autoremove -y \
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/*

ARG USERNAME=ubuntu
ARG USER_UID=1000
ARG USER_GID=$USER_UID

# the i2c group doesn't exist in the base image, but the BME680 is reached through
# /dev/i2c-1, which the host owns by group
RUN echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME \
    && { getent group i2c > /dev/null || groupadd i2c; } \
    && usermod -a -G dialout,i2c $USERNAME

USER $USERNAME
ENV USER=$USERNAME
ENV USER_WORKSPACE=/home/$USERNAME/ws_ros
WORKDIR $USER_WORKSPACE

# we copy the module into the image instead of mounting it to avoid rebuilding
# each time the container is launched (which would take a lot of time on the Pi)
COPY --chown=$USERNAME:$USERNAME ros/ src/autonomy_pi/
COPY --chown=$USERNAME:$USERNAME deps.repos colcon-defaults.yaml ./

RUN vcs import src < deps.repos \
    && sudo apt-get -q update \
    && rosdep update \
    && rosdep install -y --from-paths src --ignore-src --rosdistro ${ROS_DISTRO} \
    && sudo apt-get autoremove -y \
    && sudo apt-get clean -y \
    && sudo rm -rf /var/lib/apt/lists/*

# MAKEFLAGS defaults to a serial build because the Pi 5 runs out of memory otherwise;
# pass e.g. --build-arg MAKEFLAGS=-j4 when building on a larger machine.
ARG MAKEFLAGS=-j1
ENV COLCON_DEFAULTS_FILE=$USER_WORKSPACE/colcon-defaults.yaml
RUN . /opt/ros/${ROS_DISTRO}/setup.sh \
    && colcon build

RUN echo "source /opt/ros/${ROS_DISTRO}/setup.bash" >> /home/$USERNAME/.bashrc \
    && echo "[ -f $USER_WORKSPACE/install/setup.bash ] && source $USER_WORKSPACE/install/setup.bash" >> /home/$USERNAME/.bashrc

COPY --chown=$USERNAME:$USERNAME docker/entrypoint.sh /

ENTRYPOINT [ "/entrypoint.sh" ]
