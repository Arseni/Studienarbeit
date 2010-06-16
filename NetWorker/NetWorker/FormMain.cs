using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Net.Sockets;
using System.Net;
using System.Xml;
using System.IO;
using System.Threading;

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
            client.Client.Bind(lEndpoint);
            client.Connect(rEndpoint);
            string send = String.Format("TEL:{0}\0{1}\0", tbUnit.Text, tbCapacity.Text);
            client.Send(ASCIIEncoding.Default.GetBytes(send), send.Length);
            client.Close();
        }


        IPEndPoint rEndpoint, lEndpoint;
        private void FormMain_Load(object sender, EventArgs e)
        {
            UdpClient client = new UdpClient();
            IPEndPoint endPoint = new IPEndPoint(IPAddress.Parse("192.168.10.227"), 50001);
            client.Client.Bind(new IPEndPoint(IPAddress.Any, 50001));
            client.Connect(endPoint);
            
            XmlDocument doc = new XmlDocument();
            MemoryStream s = new MemoryStream(client.Receive(ref endPoint));
            doc.Load(s);
            
            string[] ep = doc["epm"].Attributes["home"].Value.Split(":".ToCharArray());
            int uid = int.Parse(doc["epm"].Attributes["uid"].Value);
            rEndpoint = new IPEndPoint(IPAddress.Parse(ep[0]), int.Parse(ep[1]));
            lEndpoint = new IPEndPoint(IPAddress.Any, int.Parse(ep[1]));

            client.Close();
            client = new UdpClient();
            client.Client.Bind(lEndpoint);
            client.Connect(rEndpoint);
            string tx = "ack\0";
            client.Send(ASCIIEncoding.Default.GetBytes(tx), tx.Length);
            client.Close();
        }

    }
}
