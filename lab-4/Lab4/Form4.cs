using System;
using System.Drawing;
using System.Windows.Forms;

namespace Lab4
{
    public partial class Form4 : Form
    {
        private Timer timer;
        private bool isPaused = false;
        private Button btnPauseResume;
        private Color[] colors = { Color.Red, Color.Green, Color.Blue, Color.Yellow };
        private int colorIndex = 0;

        public Form4()
        {
            InitializeComponent();
            this.Text = "Зміна кольорів";
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
                Interval = 500
            };
            timer.Tick += Timer_Tick;
            timer.Start();
        }

        private void Timer_Tick(object sender, EventArgs e)
        {
            if (!isPaused)
            {
                colorIndex = (colorIndex + 1) % colors.Length;
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
            Brush brush = new SolidBrush(colors[colorIndex]);

            e.Graphics.FillRectangle(brush, 100, 100, 50, 50);
            e.Graphics.FillEllipse(brush, 200, 100, 50, 50);
            e.Graphics.FillPolygon(brush, new PointF[]
            {
                new PointF(150, 200),
                new PointF(200, 250),
                new PointF(100, 250)
            });

            brush.Dispose();
        }

        private void Form4_Load(object sender, EventArgs e)
        {

        }
    }
}
