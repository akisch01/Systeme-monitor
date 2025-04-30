application a utiliser sur linux

Pour le son
sudo apt install sox
sox -n beep.wav synth 0.1 sine 1000

Pour python
pip install flask psutil
python3 app.py

Pour le mail
sudo apt update
sudo apt install postfix mailutils
sudo nano /etc/postfix/main.cf
sudo systemctl restart postfix

Pour lancer le projet
gcc -o monitor monitor.c
./monitor
