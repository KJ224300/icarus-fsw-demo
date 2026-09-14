#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <termios.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <math.h>

#define ORBIT_MINUTES 90
#define SUNLIT_MINUTES 60

#define UDP_PORT 5000
#define UDP_IP "127.0.0.1"

/*
 * Checks if a key has been pressed without
 * stopping the telemetry loop.
 */
int check_keyboard(void)
{
    struct termios oldt, newt;
    int oldf;
    int ch;

    /* Save the current terminal settings. */
    tcgetattr(STDIN_FILENO, &oldt);

    newt = oldt;

    /* Disable line buffering and keyboard echo. */
    newt.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    /* Make keyboard input non-blocking. */
    oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

    ch = getchar();

    /* Restore the terminal settings. */
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    fcntl(STDIN_FILENO, F_SETFL, oldf);

    return ch;
}

int main(void)
{
    /* Create a UDP socket for sending telemetry. */
    int sock = socket(AF_INET, SOCK_DGRAM, 0);

    if (sock < 0)
    {
        perror("socket");
        return 1;
    }

    /* Set the address of the Python telemetry bridge. */
    struct sockaddr_in bridge_address;

    memset(&bridge_address, 0, sizeof(bridge_address));

    bridge_address.sin_family = AF_INET;
    bridge_address.sin_port = htons(UDP_PORT);

    if (inet_pton(AF_INET, UDP_IP, &bridge_address.sin_addr) <= 0)
    {
        perror("inet_pton");
        close(sock);
        return 1;
    }

    /* These represent the nominal spacecraft state. */
    float battery = 50.0f;
    float thermal = 30.0f;

    /* These flags indicate whether an anomaly is active. */
    int thermal_anomaly = 0;
    int battery_anomaly = 0;

    /* Track the current position in the orbit. */
    int simulated_minute = 0;
    int orbit_number = 1;

    printf("ICARUS OBC simulator started.\n");
    printf("Sending telemetry to UDP %s:%d\n", UDP_IP, UDP_PORT);
    printf("Controls: [t] Thermal anomaly | [b] Battery anomaly | [r] Clear anomaly\n");

    while (1)
    {
        /* Check whether the operator has entered an anomaly command. */
        int command = check_keyboard();

        if (command == 't')
        {
            thermal_anomaly = 1;
            printf("\n*** THERMAL ANOMALY INJECTED ***\n");
        }

        if (command == 'b')
        {
            battery_anomaly = 1;
            printf("\n*** BATTERY ANOMALY INJECTED ***\n");
        }

        if (command == 'r')
        {
            /* Stop reporting the injected anomaly. */
            thermal_anomaly = 0;
            battery_anomaly = 0;

            printf("\n*** ANOMALIES CLEARED ***\n");
        }

        /*
         * Minutes 1–60 are sunlit.
         * Minutes 61–90 are in eclipse.
         *
         * Internally, the counter is 0–89.
         */
        int sunlit = simulated_minute < SUNLIT_MINUTES;

        /* Update the nominal spacecraft state based on orbital phase. */
        if (sunlit)
        {
            /* Solar panels charge the battery and thermal energy increases. */
            battery += 0.75f;
            thermal += 0.8f;
        }
        else
        {
            /* During eclipse, the battery discharges and thermal energy falls. */
            battery -= 1.5f;
            thermal -= 0.8f;
        }

        /* Keep the nominal battery value within physical limits. */
        if (battery > 100.0f)
            battery = 100.0f;

        if (battery < 0.0f)
            battery = 0.0f;

        /* Keep the nominal thermal value within the simulation limits. */
        if (thermal > 80.0f)
            thermal = 80.0f;

        if (thermal < 10.0f)
            thermal = 10.0f;

        /*
         * Telemetry normally reports the nominal spacecraft state.
         * An anomaly changes only what is reported, not the underlying
         * simulated state.
         */
        float telemetry_thermal = thermal;
        float telemetry_battery = battery;

        if (thermal_anomaly)
        {
            /* Report an extreme thermal value. */
            telemetry_thermal = 90.0f;
        }

        if (battery_anomaly)
        {
            /* Report a critically low battery level. */
            telemetry_battery = 0.0f;
        }

        /* Generate a simple varying vibration value. */
	float vibration;

	if (sunlit)
	{
    	/* Sunlit: slow, gentle oscillation. */
    		vibration =
        	0.45f
        	+ 0.08f * sin(simulated_minute * 0.12f);
	}
	else
	{
    	/* Eclipse: faster, larger oscillation. */
    		vibration =
        	0.50f
        	+ 0.25f * sin(simulated_minute * 0.8f);
	}

	if (vibration < 0.1f)
    	vibration = 0.1f;

	if (vibration > 1.0f)
    	vibration = 1.0f;
	/* Generate the telemetry timestamp. */
	struct timespec ts;
	clock_gettime(CLOCK_REALTIME, &ts);

	long long timestamp =
    		(long long)ts.tv_sec * 1000 +
    		ts.tv_nsec / 1000000;
        /* Create JSON packets for each telemetry measurement. */
        char thermal_packet[256];
        char vibration_packet[256];
        char battery_packet[256];

        snprintf(
            thermal_packet,
            sizeof(thermal_packet),
            "{\"id\":\"icarus.thermal\",\"timestamp\":%lld,\"value\":%.2f}",
            timestamp,
            telemetry_thermal
        );

        snprintf(
            vibration_packet,
            sizeof(vibration_packet),
            "{\"id\":\"icarus.vibration\",\"timestamp\":%lld,\"value\":%.2f}",
            timestamp,
            vibration
        );

        snprintf(
            battery_packet,
            sizeof(battery_packet),
            "{\"id\":\"icarus.battery\",\"timestamp\":%lld,\"value\":%.2f}",
            timestamp,
            telemetry_battery
        );

        /* Send thermal telemetry to the Python bridge. */
        sendto(
            sock,
            thermal_packet,
            strlen(thermal_packet),
            0,
            (struct sockaddr *)&bridge_address,
            sizeof(bridge_address)
        );

        /* Send vibration telemetry to the Python bridge. */
        sendto(
            sock,
            vibration_packet,
            strlen(vibration_packet),
            0,
            (struct sockaddr *)&bridge_address,
            sizeof(bridge_address)
        );

        /* Send battery telemetry to the Python bridge. */
        sendto(
            sock,
            battery_packet,
            strlen(battery_packet),
            0,
            (struct sockaddr *)&bridge_address,
            sizeof(bridge_address)
        );

        /* Display the current OBC state in the terminal. */
        printf(
            "Orbit: %d | Minute: %d | %s | Thermal: %.1f C | Vibration: %.2f g | Battery: %.1f %%\n",
            orbit_number,
            simulated_minute + 1,
            sunlit ? "SUNLIT " : "ECLIPSE",
            telemetry_thermal,
            vibration,
            telemetry_battery
        );

        /* Move to the next simulated minute. */
        simulated_minute++;

        /* After minute 90, start the next orbit at minute 1. */
        if (simulated_minute >= ORBIT_MINUTES)
        {
            simulated_minute = 0;
            orbit_number++;
        }

        /* One real second represents one simulated minute. */
	usleep(200000);
    }

    close(sock);

    return 0;
}
