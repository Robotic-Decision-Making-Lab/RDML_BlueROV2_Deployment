ARG ROS_DISTRO=lyrical
FROM ros:${ROS_DISTRO}-ros-base AS robot

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get -q update\
    && apt-get -q -y upgrade \
    && apt-get -q install --no-install-recommends -y \
    git \
    sudo \
    wget \
    && apt-get autoremove -y \
    && apt-get clean -y \
    && rm -rf /var/lib/apt/lists/*

ARG USERNAME=ubuntu
ARG USER_UID=1000
ARG USER_GID=$USER_UID

RUN echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME \
    && usermod -a -G dialout $USERNAME \
    && echo "source /usr/share/bash-completion/completions/git" >> /home/$USERNAME/.bashrc

USER $USERNAME
ENV USER=$USERNAME

RUN sudo apt-get -q update\
    && sudo apt-get -q -y upgrade \
    && sudo apt-get -q install --no-install-recommends -y \
    ros-${ROS_DISTRO}-mavros \
    ros-${ROS_DISTRO}-mavros-extras \
    && sudo apt-get autoremove -y \
    && sudo apt-get clean -y \
    && sudo rm -rf /var/lib/apt/lists/*

WORKDIR /home/$USERNAME
RUN wget https://raw.githubusercontent.com/mavlink/mavros/ros2/mavros/scripts/install_geographiclib_datasets.sh \
    && chmod +x install_geographiclib_datasets.sh \
    && sudo ./install_geographiclib_datasets.sh

COPY --chown=$USERNAME:$USERNAME mavros.yaml /home/$USERNAME
COPY --chown=$USERNAME:$USERNAME entrypoint.sh /

ENTRYPOINT [ "/entrypoint.sh" ]
