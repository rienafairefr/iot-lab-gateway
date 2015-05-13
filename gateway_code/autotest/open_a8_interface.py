#! /usr/bin/env python
# -*- coding:utf-8 -*-

""" Open A8 interface """

import shlex
import time
import datetime
from subprocess import check_output, check_call, Popen
from subprocess import STDOUT, CalledProcessError

from gateway_code.config import static_path
from gateway_code.open_node import NodeA8
from gateway_code.utils.serial_expect import SerialExpect


import logging
LOGGER = logging.getLogger('gateway_code')

_SSH_OPTS = '-F {ssh_cfg}'.format(ssh_cfg=static_path('ssh_a8_config'))
SSH_CMD = 'ssh ' + _SSH_OPTS + ' {ip_addr} "source /etc/profile; {cmd}"'
SCP_CMD = 'scp ' + _SSH_OPTS + ' {path} {ip_addr}:{remote_path}'

IP_CMD = ("ip addr show dev eth0 " +
          r" | sed -n '/inet/ s/.*inet \([^ ]*\)\/.*/\1/p'")

SOCAT_CMD = ('socat -d TCP4-LISTEN:{port},reuseaddr ' +
             'open:{tty},b{baudrate},echo=0,raw ')
LOCAL_TTY = ('socat -d tcp4-connect:{ip_addr}:{port} ' +
             ' PTY,link={tty},b{baudrate},echo=0,raw')
MAC_CMD = "cat /sys/class/net/eth0/address"


class A8ConnectionError(Exception):
    """ FatalError during tests """
    def __init__(self, value, err_msg):
        super(A8ConnectionError, self).__init__()
        self.value = value
        self.err_msg = err_msg

    def __str__(self):
        return repr(self.value) + ' : ' + repr(self.err_msg)


class OpenA8Connection(object):
    """ Connection to the Open A8, redirect A8-M3 node serial link """
    def __init__(self):
        self.ip_addr = None
        self.local_tty = None
        self.remote_tty = None

    def wait_boot_and_get_ip_address(self):
        """ Wait until open node is booted and get its ip address"""
        a8_serial = SerialExpect(NodeA8.TTY, NodeA8.BAUDRATE, LOGGER)

        LOGGER.debug("Time before boot %s", datetime.datetime.now())
        # boot timeout 5 minutes (seen likely 3minutes loaded server)
        ret = a8_serial.expect(' login: ', timeout=300)
        LOGGER.debug("Time after boot %s", datetime.datetime.now())
        if not ret:  # pragma: no cover
            raise A8ConnectionError("Open node didn't booted: %r" % ret,
                                    'boot_timeout')

        a8_serial.send('root')
        a8_serial.expect('# ')

        a8_serial.send(IP_CMD)
        ip_address = a8_serial.expect(r'\d+\.\d+\.\d+.\d+', timeout=10)
        if not ret:  # pragma: no cover
            raise A8ConnectionError("Invalid Ip address",
                                    "invalid_ip:%r" % ip_address)
        a8_serial.send('exit')

        self.ip_addr = ip_address
        # I should do some ping to allow ssh to work
        # maybe because of the restriction on iptables, but it works
        ping_out = check_output(shlex.split('ping -q -c 5 %s' % self.ip_addr))
        LOGGER.debug(ping_out)

    def start(self):
        """ Start a redirection of open_A8 M3 node serial """
        port = 20000

        # wait until boot
        self.wait_boot_and_get_ip_address()

        try:
            # first run an empty command to prevent warning message
            # "Warning: Permanently added '192.168.1.6' (RSA)
            #  to the list of known hosts.\r\n"
            self.ssh_run('echo "ssh ok"')
        except CalledProcessError as err:  # pragma: no cover
            raise A8ConnectionError(
                "Could not ssh connect to openA8 %s" % str(err),
                "open_a8_ssh_connection_failed")

        try:
            output = self.ssh_run('cat /tmp/boot_errors')
        except CalledProcessError:
            pass  # file does not exist, don't care
        else:
            if len(output):  # pragma: no cover
                raise A8ConnectionError("Open A8 FTDI config failed", output)

        # test if config OK for OPEN A8 m3
        output = self.ssh_run('ftdi-devices-list')
        if 'A8-M3' not in output:  # pragma: no cover
            raise A8ConnectionError("Open A8 doesn't have M3 configured",
                                    "Open_A8_m3_ftdi_not_configured")

        # remote TTY
        socat_cmd = SOCAT_CMD.format(port=port,
                                     tty=NodeA8.A8_M3_TTY,
                                     baudrate=NodeA8.A8_M3_BAUDRATE)
        remote_tty_cmd = 'killall socat 2>/dev/null; ' + socat_cmd
        cmd = SSH_CMD.format(ip_addr=self.ip_addr, cmd=remote_tty_cmd)
        LOGGER.debug(cmd)
        self.remote_tty = Popen(shlex.split(cmd))
        time.sleep(10)

        # local_tty
        local_tty = LOCAL_TTY.format(ip_addr=self.ip_addr, port=port,
                                     tty=NodeA8.LOCAL_A8_M3_TTY,
                                     baudrate=NodeA8.A8_M3_BAUDRATE)

        self.local_tty = Popen(shlex.split(local_tty))
        time.sleep(2)

        if (self.local_tty.poll() is not None or
                self.remote_tty.poll() is not None):  # pragma: no cover
            self.remote_tty.terminate()
            self.local_tty.terminate()
            raise A8ConnectionError("Socat commands shut down too early",
                                    "socat_terminated")

    def stop(self):
        """ Stop redirection of open_A8 M3 node serial """
        try:
            self.ssh_run('killall socat')
            time.sleep(1)
            self.remote_tty.wait()
            self.local_tty.wait()
        except CalledProcessError:
            # Open node A8 unreachable
            if self.remote_tty is not None:  # pragma: no cover
                self.remote_tty.terminate()
            if self.local_tty is not None:  # pragma: no cover
                self.local_tty.terminate()
        LOGGER.debug("a8_connection_stopped")

    def get_mac_addr(self):
        """ Get eth0 mac address """
        mac_addr_s = self.ssh_run(MAC_CMD)
        mac_addr = mac_addr_s.strip()
        return mac_addr

    def ssh_run(self, command):
        """ Run SSH command on A8 node """
        cmd = SSH_CMD.format(ip_addr=self.ip_addr, cmd=command)
        LOGGER.debug(cmd)

        output = check_output(shlex.split(cmd), stderr=STDOUT)
        return output

    def scp(self, src, dest):
        """ SCP scr to A8 node at dest """
        cmd = SCP_CMD.format(ip_addr=self.ip_addr, path=src, remote_path=dest)
        LOGGER.debug(cmd)
        check_call(shlex.split(cmd))
