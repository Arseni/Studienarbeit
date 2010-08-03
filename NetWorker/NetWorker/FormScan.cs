using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Windows.Forms;

namespace NetWorker
{
    public partial class FormScan : Form
    {
        public FormScan()
        {
            InitializeComponent();
        }

        private void FormScan_Load(object sender, EventArgs e)
        {
            EpmDevice.OnDeviceFound += new EventHandler<EpmDeviceFoundEventArgs>(EpmDevice_OnDeviceFound);
            EpmDevice.SearchEpmDeviceStart();
        }

        void EpmDevice_OnDeviceFound(object sender, EpmDeviceFoundEventArgs e)
        {
            if (listBox1.InvokeRequired)
            {
                listBox1.Invoke(new EventHandler<EpmDeviceFoundEventArgs>(EpmDevice_OnDeviceFound), new object[] { sender, e });
            }
            else
            {
                listBox1.Items.Add(e.device);
            }
        }

        private void FormScan_FormClosed(object sender, FormClosedEventArgs e)
        {
            EpmDevice.SearchEpmDeviceStop();
        }

        private void listBox1_SelectedIndexChanged(object sender, EventArgs e)
        {
            EpmDeviceDescription sel = (EpmDeviceDescription)listBox1.SelectedItem;
            if (sel != null)
            {
                textBox1.Text = String.Format("IP Adresse: {0}\r\nPort: {1}\r\n\r\nUnits:", sel.remoteIPAddr, sel.remotePort);
                foreach (string i in sel.units)
                    textBox1.AppendText("\r\n - " + i);
                buttonConnect.Enabled = true;
            }
        }

        private void buttonConnect_Click(object sender, EventArgs e)
        {
            new FormSingleDevice((EpmDeviceDescription)listBox1.SelectedItem).Show();
            this.Close();
        }

        private void buttonCancel_Click(object sender, EventArgs e)
        {
            Application.Exit();
        }
    }
}
