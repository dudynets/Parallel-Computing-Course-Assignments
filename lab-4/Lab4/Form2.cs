using System;
using System.Drawing;
using System.Windows.Forms;

namespace Lab4
{
    public partial class Form2 : Form
    {
        private Timer timer;
        private bool isPaused = false;
        private Button btnPauseResume;
        private int rectWidth = 50;
        private int rectHeight = 50;
        private int deltaWidth = 2;
        private int deltaHeight = -2;

        public Form2()
        {
            InitializeComponent();
            this.Text = "Прямокутник змінює розміри";
            this.ControlBox = false;
            this.ClientSize = new Size(400, 400);
            this.DoubleBuffered = true;

            btnPauseResume = new Button
            {
                Text = "Призупинити",
                Location = new Point(10, 10),
                AutoSize = true
            };
            btnPauseResume.Click += BtnPauseResume_Click;
            this.Controls.Add(btnPauseResume);

            timer = new Timer
            {
                Interval = 30
            };
            timer.Tick += Timer_Tick;
            timer.Start();
        }

        private void Timer_Tick(object sender, EventArgs e)
        {
            if (!isPaused)
            {
                rectWidth += deltaWidth;
                rectHeight += deltaHeight;

                if (rectWidth > 200 || rectWidth < 50)
                    deltaWidth = -deltaWidth;
                if (rectHeight > 200 || rectHeight < 50)
                    deltaHeight = -deltaHeight;

                this.Invalidate();
            }
        }

        private void BtnPauseResume_Click(object sender, EventArgs e)
        {
            isPaused = !isPaused;
            btnPauseResume.Text = isPaused ? "Продовжити" : "Призупинити";
        }

        protected override void OnPaint(PaintEventArgs e)
        {
            base.OnPaint(e);
            int x = (this.ClientSize.Width - rectWidth) / 2;
            int y = (this.ClientSize.Height - rectHeight) / 2;
            e.Graphics.FillRectangle(Brushes.Blue, x, y, rectWidth, rectHeight);
        }

        private void Form2_Load(object sender, EventArgs e)
        {

        }
    }
}
