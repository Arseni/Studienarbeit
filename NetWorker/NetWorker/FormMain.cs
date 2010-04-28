using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Net.Sockets;

namespace NetWorker
{
    public partial class FormMain : Form
    {
        public FormMain()
        {
            InitializeComponent();
        }

        private void button1_Click(object sender, EventArgs e)
        {
            TcpClient client = new TcpClient();
            client.Connect("192.168.0.13", 1000);

            client.Close();
        }

        private void button2_Click(object sender, EventArgs e)
        {
            UdpClient client = new UdpClient();
            client.Connect("192.168.0.13", 1001);
            client.Send(new byte[] { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }, 10);
            client.Close();
        }
    }
}
