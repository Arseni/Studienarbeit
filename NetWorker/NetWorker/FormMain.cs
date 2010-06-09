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
            client.Connect("192.168.10.227", 1100);

            client.Close();
        }

        private void button2_Click(object sender, EventArgs e)
        {
            UdpClient client = new UdpClient();
            client.Connect("192.168.10.227", 1101);
            client.Send(ASCIIEncoding.Default.GetBytes("Buttons\0"), 8);
            client.Close();
        }
    }
}
