using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.IO;
using System.Xml;

namespace NetWorker
{
    public partial class FormSingleDevice : Form
    {

        IPEndPoint rEndpoint, lEndpoint;
        public FormSingleDevice()
        {
            InitializeComponent();

            rEndpoint = new IPEndPoint(IPAddress.Any, 50000);
            lEndpoint = new IPEndPoint(IPAddress.Any, 50000);
        }

        public FormSingleDevice(EpmDeviceDescription target)
            : this()
        {
            rEndpoint = new IPEndPoint(target.remoteIPAddr, target.remotePort);
            Send("<epm><unit><ack/></unit></epm>");
        }

        private void FormSingleDevice_FormClosed(object sender, FormClosedEventArgs e)
        {
            if(receiver != null)
                receiver.Abort();
            Application.Exit();
        }


        void Send(string text)
        {
            UdpClient client = new UdpClient();
            client.Client.Bind(lEndpoint);
            byte[] tx = ASCIIEncoding.Default.GetBytes(text+"\0");

            client.Send(tx, tx.Length, rEndpoint);
            client.Close();
        }

        bool SendWithAck(string text)
        {
            UdpClient client = new UdpClient();
            client.Client.Bind(lEndpoint);
            bool ret = false;
            byte[] tx = ASCIIEncoding.Default.GetBytes(text + "\0");

            client.Send(tx, tx.Length, rEndpoint);

            client.Client.ReceiveTimeout = 1000;
            try
            {
                Stream s = new MemoryStream(client.Receive(ref rEndpoint));
                XmlDataDocument doc = new XmlDataDocument();

                doc.Load(s);
                if (doc["epm"]["unit"]["ack"].Attributes["cmd"].Value.Length > 0)
                    ret = true;
            }
            catch{ }
            
            client.Close();
            return ret;
        }

        private void button1_Click(object sender, EventArgs e)
        {
            XmlDocument doc = new XmlDocument();
            doc.Load(new MemoryStream(ASCIIEncoding.Default.GetBytes("<epm><unit name=\"SerCom\"><write data=\"string\">" + textBoxSend.Text + "</write></unit></epm>")));

            if (checkBox1.Checked)
            {
                doc["epm"].SetAttribute("ack", "yes");
                if (SendWithAck(doc.InnerXml))
                    pictureBox2.Image = imageListStatus.Images["ok"];
                else
                {
                    pictureBox2.Image = imageListStatus.Images["fail"];
                    return;
                }
            }
            else
            {
                Send(doc.InnerXml);
                pictureBox2.Image = imageListStatus.Images["none"];
            }
            listboxUpdate(rxSource.send, DateTime.Now, doc["epm"]);
            
        }

        Thread receiver = null;
        private void button2_Click(object sender, EventArgs e)
        {
            receiver = new Thread(new ParameterizedThreadStart(rx));
            receiver.Start(int.Parse(textBoxPort.Text));
            if(SendWithAck("<epm ack=\"yes\" reply=\"onchange\" dest=\"192.168.10.222:"+textBoxPort.Text+"\"><unit name=\"SerCom\"><read stx=\"" + (int)ASCIIEncoding.Default.GetBytes(textBoxSTX.Text)[0] + "\" etx=\"" + (int)ASCIIEncoding.Default.GetBytes(textBoxETX.Text)[0] + "\"/></unit></epm>"))
                pictureBox1.Image = imageListStatus.Images["ok"];
            else
                pictureBox1.Image = imageListStatus.Images["fail"];
        }

        void rx(object port)
        {
            UdpClient client = new UdpClient();
            IPEndPoint r = new IPEndPoint(IPAddress.Any, 50000);
            client.Client.Bind(new IPEndPoint(IPAddress.Any, (int)port));
            client.Client.ReceiveTimeout = 1000;
            while (true)
            {
                try
                {
                    Stream s = new MemoryStream(client.Receive(ref r));
                    XmlDataDocument doc = new XmlDataDocument();

                    doc.Load(s);
                    listboxUpdate(rxSource.asyncReceive, DateTime.Now, doc["epm"]);
                }
                catch { }
            }
        }

        delegate void listboxUpdater(rxSource source, DateTime now, XmlElement data);
        void listboxUpdate(rxSource source, DateTime now, XmlElement data)
        {
            if(dataGridView1.InvokeRequired)
            {
                dataGridView1.Invoke(new listboxUpdater(listboxUpdate), new object[] { source, now, data });
            }
            else
            {
                Image src = null;
                switch (source)
                {
                    case rxSource.asyncReceive :
                        src = imageListSource.Images["event"];
                        break;
                    case rxSource.send :
                        src = imageListSource.Images["local"];
                        break;
                    case rxSource.syncReceive:
                        src = imageListSource.Images["remote"];
                        break;
                }
                dataGridView1.Rows.Add(new object[] { src, now.ToLongTimeString(), data.InnerText, data.InnerXml });
            }
        }
    }
    enum rxSource
    {
        asyncReceive,
        syncReceive,
        send
    }
}
