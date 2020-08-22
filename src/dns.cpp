/* -*- mode: C++; -*-
 *
 * DNS driven out of the task scheduler
 */

#include <DNSServer.h>
#include <ESP8266WiFi.h>

#include <espy.h>

const byte DNS_PORT = 53;

DNSServer dns = DNSServer();

void dns_task();

Task dnsTask(10, TASK_FOREVER, &dns_task);

void dns_setup(Scheduler &scheduler) {
    scheduler.addTask(dnsTask);
}

void dns_enable() {
    dns.start(DNS_PORT, "*", WiFi.softAPIP());

    dnsTask.enable();
}

void dns_disable() {
    dns.stop();
    dnsTask.disable();
}

void dns_task() {
    dns.processNextRequest();
}
