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
            UdpClient client = new UdpClient();
            client.Client.Bind(lEndpoint);
            client.Connect(rEndpoint);
            MessageBox.Show(ASCIIEncoding.Default.GetString(client.Receive(ref rEndpoint)));
            client.Close();
        }

        private void button2_Click(object sender, EventArgs e)
        {
            UdpClient client = new UdpClient();
            client.Client.Bind(lEndpoint);
            client.Connect(rEndpoint);
            string send = buildXMLString();
            client.Send(ASCIIEncoding.Default.GetBytes(send), send.Length);
            while (true)
            {
                Stream s = new MemoryStream(client.Receive(ref rEndpoint));
                XmlDataDocument doc = new XmlDataDocument();
                try
                {
                    doc.Load(s);
                    string txt = doc.InnerXml;
                    MessageBox.Show(txt);//String.Format("{0} - {1}\r\n", doc["epm"]["unit"].Attributes["name"].Value,
                                          //                             doc["epm"]["unit"].InnerXml));
                }
                catch { }
            }
            client.Close();
        }

        private string buildXMLString()
        {
            return "<epm ack=\"yes\" uid=\"5544\" withseqno=\"yes\"><unit name=\"Buttons\"><ButtonState sendonarrival=\"yes\"/></unit></epm>\0";
        }


        IPEndPoint rEndpoint, lEndpoint;
        private void FormMain_Load(object sender, EventArgs e)
        {          
            //UdpClient client = new UdpClient();
            rEndpoint = new IPEndPoint(IPAddress.Parse("192.168.0.13"/*10.227"*/), 50001);
            lEndpoint = new IPEndPoint(IPAddress.Any, 50001);
            //client.Client.Bind(lEndpoint);
           // client.Connect(rEndpoint);
           // client.Close();

            /*XmlDocument doc = new XmlDocument();
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
            client.Close();*/
        }

        private void button3_Click(object sender, EventArgs e)
        {
            UdpClient client = new UdpClient();
            client.Client.Bind(lEndpoint);
            client.Connect(rEndpoint);
            string send = "<epm><unit><ack/></unit></epm>\0";
            client.Send(ASCIIEncoding.Default.GetBytes(send), send.Length);
            client.Close();
        }

    }
}
