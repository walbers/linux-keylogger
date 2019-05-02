A basic keylogger with a TCP connection for older linux kernel's.
Does not require root.
The bash script will execute cowroot.c which uses the dirty cow exploit to gain root. It will then install the kernel module which records keystrokes and sends them to the server.

# Running the keylogger
1. Downgrade Kernel
2. Change the IP in combo.c on line 264 to the desired ip (default is localhost)
2. Compile and run the test server
3. Run the bash script

# Downgrading Kernel - ubuntu
1. Install Ukuu
2. Install kernel version 4.3.0
3. Restart computer while pressing ESC when the bios shows
4. Go into Advanced Ubuntu Options
5. Select correct kernel version 

## Project Collaborators
Will Albers, Sayan Bhattacharjee, Ishan Shah (mentor), Ophir Sneh (mentor)
