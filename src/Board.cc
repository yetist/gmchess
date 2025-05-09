/*
 * Board.h
 * Copyright (C) wind 2009 <xihels@gmail.com>
 *
 */

#include "Board.h"
#include <vector>
#include <string.h>
#include <cassert>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include "Engine.h"
#include "MainWindow.h"
#include "Sound.h"
#include "ec_throw.h"

/** 边界的宽度*/
/**  width of border */
const int border_width = 32;
/** 棋子的宽度*/
/** width of chessman */
//const int chessman_width = 57;
//const int chessman_width = 29;

#define PLACE_LEFT 0x01
#define PLACE_RIGHT 0x02
#define PIECE_START 16  //棋子开始数字 start number of chessman
#define PIECE_END   48  //棋子结束数字 end number of chessman
#define PLACE_ALL PLACE_LEFT | PLACE_RIGHT
#define IMGAGE_DIR DATA_DIR"/themes/west/"
#define IMGAGE_SMALL_DIR DATA_DIR"/themes/west-small/"
#define HEX_ESCAPE '%'

int hex_to_int (gchar c);
int unescape_character (const char *scanner);
std::string  wind_unescape_string (const char *escaped_string,
    const gchar *illegal_characters);

int hex_to_int (gchar c)
{
	return  c >= '0' && c <= '9' ? c - '0'
		: c >= 'A' && c <= 'F' ? c - 'A' + 10
		: c >= 'a' && c <= 'f' ? c - 'a' + 10
		: -1;
}

int unescape_character (const char *scanner)
{
	int first_digit;
	int second_digit;

	first_digit = hex_to_int (*scanner++);
	if (first_digit < 0) {
		return -1;
	}

	second_digit = hex_to_int (*scanner++);
	if (second_digit < 0) {
		return -1;
	}

	return (first_digit << 4) | second_digit;
}

/** 用于在拖放时得到的文件名的转码*/
/** get the code of when drag the filename*/
std::string  wind_unescape_string (const char *escaped_string,
		const gchar *illegal_characters)
{
	const char *in;
	char *out;
	int character;

	if (escaped_string == NULL) {
		return std::string();
	}

	//result = g_malloc (strlen (escaped_string) + 1);
	char result[strlen (escaped_string) + 1];

	out = result;
	for (in = escaped_string; *in != '\0'; in++) {
		character = *in;
		if (*in == HEX_ESCAPE) {
			character = unescape_character (in + 1);

			/* Check for an illegal character. We consider '\0' illegal here. */
			if (character <= 0
					|| (illegal_characters != NULL
						&& strchr (illegal_characters, (char)character) != NULL)) {
				return std::string();
			}
			in += 2;
		}
		*out++ = (char)character;
	}

	*out = '\0';
	assert ((size_t)(out - result) <= strlen (escaped_string));
	return std::string(result);

}

Board::Board(MainWindow& win) : parent(win)
{

#if 0
	std::list<Gtk::TargetEntry> listTargets;
	listTargets.push_back(Gtk::TargetEntry("STRING"));
	listTargets.push_back(Gtk::TargetEntry("text/plain"));


	this->set_size_request(221,277);

	this->drag_dest_set(listTargets);
	this->signal_drag_data_received().connect(
			sigc::mem_fun(*this, &Board::on_drog_data_received));
#endif

	/** 加载所需要图片进内存*/
	load_images();

	p_pgnfile=new Pgnfile(m_engine);
	m_engine.init_snapshot(start_fen);
	m_robot.set_out_slot(sigc::mem_fun(*this, &Board::robot_log));
	this->set_events(Gdk::BUTTON_PRESS_MASK|Gdk::EXPOSURE_MASK);

	this->show_all();

}


Board::~Board()
{
	if(timer.connected())
		timer.disconnect();
	m_robot.send_ctrl_command("quit\n");
	m_robot.stop();
}

void Board::set_trace_color(const std::string& color_)
{
	color = color_;
}
void Board::set_themes(const std::string& themes_)
{
	theme = themes_;
	/** 加载所需要图片进内存*/
	load_images();
	queue_draw();
}

Cairo::RefPtr<Cairo::ImageSurface> Board::get_pic (const std::string &name_)
{
	const std::string path = std::string (DATA_DIR) +
	  "/themes/" + theme + "/" + name_;
	return Cairo::ImageSurface::create_from_png (path);
}

Cairo::RefPtr<Cairo::ImageSurface> Board::get_spic (const std::string &name_)
{
	const std::string path = std::string (DATA_DIR) +
	  "/themes/" + theme + "-small/" + name_;
	return Cairo::ImageSurface::create_from_png (path);
}

void Board::load_images()
{
	bg_image = get_pic("bg.png");
	if(is_small_board){
		chessman_images[BLACK_ADVISOR] = get_spic("black_advisor.png");
		chessman_images[BLACK_BISHOP] = get_spic("black_bishop.png");
		chessman_images[BLACK_CANNON] = get_spic("black_cannon.png");
		chessman_images[BLACK_KING] = get_spic("black_king.png");
		chessman_images[BLACK_KING_DIE] = get_spic("black_king_die.png");
		chessman_images[BLACK_KNIGHT] = get_spic("black_knight.png");
		chessman_images[BLACK_PAWN] = get_spic("black_pawn.png");
		chessman_images[BLACK_ROOK] = get_spic("black_rook.png");
		chessman_images[RED_ADVISOR] = get_spic("red_advisor.png");
		chessman_images[RED_BISHOP] = get_spic("red_bishop.png");
		chessman_images[RED_CANNON] = get_spic("red_cannon.png");
		chessman_images[RED_KING] = get_spic("red_king.png");
		chessman_images[RED_KING_DIE] = get_spic("red_king_die.png");
		chessman_images[RED_KNIGHT] = get_spic("red_knight.png");
		chessman_images[RED_PAWN] = get_spic("red_pawn.png");
		chessman_images[RED_ROOK] = get_spic("red_rook.png");
		chessman_images[SELECTED_CHESSMAN] = get_spic("select.png");
		chessman_images[PROPMT] = get_spic("null.png");
	}
	else{

		chessman_images[BLACK_ADVISOR] = get_pic("black_advisor.png");
		chessman_images[BLACK_BISHOP] = get_pic("black_bishop.png");
		chessman_images[BLACK_CANNON] = get_pic("black_cannon.png");
		chessman_images[BLACK_KING] = get_pic("black_king.png");
		chessman_images[BLACK_KING_DIE] = get_pic("black_king_die.png");
		chessman_images[BLACK_KNIGHT] = get_pic("black_knight.png");
		chessman_images[BLACK_PAWN] = get_pic("black_pawn.png");
		chessman_images[BLACK_ROOK] = get_pic("black_rook.png");
		chessman_images[RED_ADVISOR] = get_pic("red_advisor.png");
		chessman_images[RED_BISHOP] = get_pic("red_bishop.png");
		chessman_images[RED_CANNON] = get_pic("red_cannon.png");
		chessman_images[RED_KING] = get_pic("red_king.png");
		chessman_images[RED_KING_DIE] = get_pic("red_king_die.png");
		chessman_images[RED_KNIGHT] = get_pic("red_knight.png");
		chessman_images[RED_PAWN] = get_pic("red_pawn.png");
		chessman_images[RED_ROOK] = get_pic("red_rook.png");
		chessman_images[SELECTED_CHESSMAN] = get_pic("select.png");
		chessman_images[PROPMT] = get_pic("null.png");
	}
}

void Board::configure_board(int _width)
{
	if(is_small_board && _width >=521){
		is_small_board=false;
		chessman_width=57;
		load_images();
		queue_draw();

	}
	if(!is_small_board && _width<521){
		is_small_board=true;
		chessman_width=29;
		load_images();
		queue_draw();
	}

}

void Board::get_grid_size(int& width, int& height)
{
	width = (get_width() - border_width * 2) / 8;
	height = (get_height() - border_width * 2) / 9;
}

Gdk::Point Board::get_coordinate(int pos_x, int pos_y)
{
	int grid_width;
	int grid_height;
	get_grid_size(grid_width, grid_height);
	pos_x = pos_x * grid_width + border_width;
	pos_y = pos_y * grid_height + border_width;

	return Gdk::Point(pos_x, pos_y);
}


Gdk::Point Board::get_position(int pos_x, int pos_y)
{
	int grid_width;
	int grid_height;
	get_grid_size(grid_width, grid_height);
	pos_x -= border_width;
	pos_y -= border_width;
	pos_x += grid_width / 2;
	pos_y += grid_height / 2;
	int x = pos_x / grid_width;
	int y = pos_y / grid_height;
	int offset_x = pos_x - x * grid_width;
	int offset_y = pos_y - y * grid_height;
	if ((offset_x > chessman_width ) || (offset_y > chessman_width ))
		return Gdk::Point(-1, -1);
	return Gdk::Point(x, y);
}

bool Board::on_draw (const ::Cairo::RefPtr<::Cairo::Context> &cr)
{
	draw_bg();
	int mv = m_engine.get_last_move_from_snapshot();
	if (mv > 0) {
	    draw_trace(mv);
	}
	draw_board();
	draw_select_frame(true);
	draw_show_can_move();
	return true;
}

/**处理点击事件, handle the click events*/
bool Board::on_button_press_event(GdkEventButton* ev)
{
	/** 对战状态下，电脑走棋时就不响应鼠标事件了*/
	/** if fight with AI, the compute doesn't handle the events */
	if(is_fight_to_robot()||is_network_game()){
		if(!is_human_player())
			return true;
	}
	if(ev->type == GDK_BUTTON_PRESS&& ev->button == 1)
	{
		Gdk::Point p = get_position(ev->x, ev->y);
		selected_x = p.get_x();
		selected_y = p.get_y();
		if(selected_chessman == -1){
			/** 之前没选中棋子，现在选择 */
			/** there is not select chessman,now select*/

			CSound::play(SND_CHOOSE);
			if (selected_x != -1) {
				selected_chessman = m_engine.get_piece(selected_x, selected_y,is_rev_board);
				if(selected_chessman==0){
					/** 仍然没选中, still not select */
					selected_chessman = -1;
					printf("still no select chessman\n");
					return true;
				}
				/** 对战状态中，选了对方棋子无效*/
				/** choose the enemy chessman is useless on war */
				if(is_fight_to_robot()||is_network_game()){
					if((m_human_black && (selected_chessman <32))||((!m_human_black)&&(selected_chessman>31))){
						printf("choose black %d\n",selected_chessman);
						selected_chessman =-1;
						return true;
					}
				}

				queue_draw();
			}
		}
		else{
			/** 之前已经选中棋子，现在是生成着法或取消*/
			/** there has a selecter, now buider the moves or canel */
			int dst_chessman = m_engine.get_piece(selected_x,selected_y,is_rev_board);
			if( (dst_chessman!=0) && ((selected_chessman &16)==(dst_chessman&16))){
				/** 之前所选及现在选是同一色棋子, 变更棋子选择 */
				/** change the select */
				selected_chessman = dst_chessman;
				CSound::play(SND_CHOOSE);
				queue_draw();

			}
		//else if(dst_chessman == 0){
		else{
				/** 目标地点没有棋子可以直接生成着法，当然还需要检测一下从源地点到终点是否是合法的着法，交由下面着法生成函数负责*/
				/** 目标地点有对方棋子，其实也可以给着法生成函数搞啊*/
				try_move(selected_x,selected_y);

			}


		}
	}
	else if(ev->type == GDK_BUTTON_PRESS&& ev->button == 3){
		/** 右键取消选择*/
		/** the right click canel the choose*/
		selected_chessman = -1;
		queue_draw();

	}
	return true;
}

void Board::draw_bg()
{
	Gdk::Point p1= get_coordinate(0, 0);
	Gdk::Point p2= get_coordinate(8, 9);

	int width = p2.get_x() - p1.get_x();
	int height = p2.get_y() - p1.get_y();

	Glib::RefPtr<Gdk::Window> window = get_window ();
	Cairo::RefPtr<Cairo::Context> cr = window->create_cairo_context ();

	Cairo::RefPtr<Cairo::ImageSurface> surface;
	Cairo::RefPtr<Cairo::Context> bgcr;

	surface = Cairo::ImageSurface::create(Cairo::Format::FORMAT_ARGB32, width * 9, height * 10);
	bgcr = Cairo::Context::create(surface);

	Cairo::RefPtr<Cairo::Pattern> pattern = Cairo::SurfacePattern::create(bg_image);
	pattern->set_extend(Cairo::Extend::EXTEND_REPEAT);
	bgcr->set_source(pattern);
	bgcr->fill();
	bgcr->paint();

	cr->set_source(surface, 0, 0);
	cr->paint();

	// 画棋盘外框
	cr->save();
	cr->set_source_rgb(0, 0, 0);
	cr->set_line_width(4.0);
	cr->rectangle(p1.get_x () - 8, p1.get_y () - 8, width + 8 * 2, height + 8 * 2);
	cr->stroke();

	// 画棋盘内框
	cr->set_line_width(2.0);
	cr->rectangle(p1.get_x (), p1.get_y (), width, height);
	cr->stroke();

	cr->set_line_width(1.0);
	int grid_width;
	int grid_height;
	get_grid_size(grid_width, grid_height);

	// 画棋盘横线
	for (int i = 0; i < 9; i++)
	{
		p1 = get_coordinate (0, i);
		p2 = get_coordinate (8, i);
		cr->move_to(p1.get_x(), p1.get_y());
		cr->line_to(p2.get_x(), p2.get_y());
		cr->stroke();
	}

	// 画上方棋盘纵线
	for (int i = 0; i < 8; i++)
	{
		p1 = get_coordinate (i, 0);
		p2 = get_coordinate (i, 4);
		cr->move_to(p1.get_x(), p1.get_y());
		cr->line_to(p2.get_x(), p2.get_y());
		cr->stroke();
	}

	// 画下方棋盘纵线
	for (int i = 0; i < 8; i++)
	{
		p1 = get_coordinate (i, 5);
		p2 = get_coordinate (i, 9);
		cr->move_to(p1.get_x(), p1.get_y());
		cr->line_to(p2.get_x(), p2.get_y());
		cr->stroke();
	}

	cr->set_line_width(2.0);    // make the line wider
	draw_localize (cr, 0, 3, PLACE_LEFT);
	draw_localize (cr, 8, 3, PLACE_RIGHT);

	for (int i = 0; i < 3; i++) {
		draw_localize(cr, i * 2 + 2, 3, PLACE_ALL);
	}

	draw_localize(cr, 1, 2, PLACE_ALL);
	draw_localize(cr, 7, 2, PLACE_ALL);

	draw_localize(cr, 0, 6, PLACE_LEFT);
	draw_localize(cr, 8, 6, PLACE_RIGHT);

	for (int i = 0; i < 3; i++) {
		draw_localize(cr, i * 2 + 2, 6, PLACE_ALL);
	}

	draw_localize(cr, 1, 7, PLACE_ALL);
	draw_localize(cr, 7, 7, PLACE_ALL);

	cr->set_line_width(1.0);
	draw_palace (cr, 4, 1);
	draw_palace (cr, 4, 8);
}

void Board::draw_localize(Cairo::RefPtr<Cairo::Context> &cr, int x, int y, int place)
{
	int width;
	int height;
	get_grid_size(width, height);
	width /= 5;
	height /= 5;

	Gdk::Point p = get_coordinate(x, y);

	if (place & PLACE_LEFT) {
		cr->move_to(p.get_x() + 5, p.get_y () - height - 4);
		cr->line_to(p.get_x () + 5, p.get_y () - 4);
		cr->line_to(p.get_x () + 5 + width, p.get_y () - 4);
		cr->stroke();

		cr->move_to(p.get_x () + 5 + width, p.get_y () + 5);
		cr->line_to(p.get_x () + 5, p.get_y () + 5);
		cr->line_to(p.get_x () + 5, p.get_y () + 5 + height);
		cr->stroke();
	}

	if (place & PLACE_RIGHT) {
		cr->move_to(p.get_x () - 4 - width, p.get_y () - 4);
		cr->line_to(p.get_x () - 4, p.get_y () - 4);
		cr->line_to(p.get_x () - 4, p.get_y () - 4 - height);
		cr->stroke();

		cr->move_to(p.get_x () - 4 - width, p.get_y () + 5);
		cr->line_to(p.get_x () - 4, p.get_y () + 5);
		cr->line_to(p.get_x () - 4, p.get_y () + 5 + height);
		cr->stroke();
	}
}

void Board::draw_palace(Cairo::RefPtr<Cairo::Context> &cr, int x, int y)
{
	int width;
	int height;
	get_grid_size(width, height);
	Gdk::Point p = get_coordinate(x, y);

	cr->move_to(p.get_x () - width, p.get_y () - height);
	cr->line_to(p.get_x () + width, p.get_y () + height);
	cr->move_to(p.get_x () + width, p.get_y () - height);
	cr->line_to(p.get_x () - width, p.get_y () + height);
	cr->stroke();
}

void Board::draw_chessman(int x, int y, int chessman)
{


	int chess_type = m_engine.get_chessman_type(chessman);
	if(chess_type<0||chess_type>13)
		return;

	Gdk::Point p = get_coordinate(x, y);
	int px = p.get_x() - chessman_width / 2;
	int py = p.get_y() - chessman_width / 2;

	Glib::RefPtr<Gdk::Window> window = get_window ();
	Cairo::RefPtr<Cairo::Context> cr = window->create_cairo_context ();

	cr->set_source(chessman_images[chess_type], px, py);
	cr->paint();
}

void Board::draw_show_can_move()
{
	if(!is_fight_to_robot()&& !is_network_game())
		return;

	if (selected_chessman < 0 )
		return;
	std::vector<Gdk::Point> points;
	m_engine.gen_which_can_move(points, selected_chessman, is_rev_board);

	std::vector<Gdk::Point>::iterator iter = points.begin();

	for(;iter != points.end(); ++iter){
		Gdk::Point p = get_coordinate(iter->get_x(), iter->get_y());
		draw_phonily_point(p);
	}

}

void Board::draw_phonily_point(Gdk::Point& p)
{
	int px = p.get_x() - 11 / 2;
	int py = p.get_y() - 11 / 2;

	Glib::RefPtr<Gdk::Window> window = get_window ();
	Cairo::RefPtr<Cairo::Context> cr = window->create_cairo_context ();

	cr->set_source(chessman_images[PROPMT], px, py);
	cr->paint();
}

void Board::draw_select_frame(bool selected)
{
	if (selected_chessman < 0 || selected_x == -1 || selected_y == -1)
		return;

	/** 目前要做的是根据棋子代号，获取它所在的棋盘9x10坐标*/
	int sx,sy;
	m_engine.get_xy_from_chess(selected_chessman,sx,sy,is_rev_board);
	Gdk::Point p = get_coordinate(sx, sy);


	int px = p.get_x() - chessman_width / 2;
	int py = p.get_y() - chessman_width / 2;

	Glib::RefPtr<Gdk::Window> window = get_window ();
	Cairo::RefPtr<Cairo::Context> cr = window->create_cairo_context ();
	if (selected) {
		cr->set_source(chessman_images[SELECTED_CHESSMAN], px, py);
		cr->paint();
	}
}


void Board::draw_board()
{
	for (int i = 0; i < 9; i++) {
		for (int j = 0; j < 10; j++) {
			draw_chessman(i, j, m_engine.get_piece(i, j, is_rev_board));
		}
	}
}

void Board::draw_trace(int mv)
{
	int src = m_engine.get_move_src(mv);
	int dst = m_engine.get_move_dst(mv);

	Glib::RefPtr<Gdk::Window> window = get_window ();
	Cairo::RefPtr<Cairo::Context> cr = window->create_cairo_context ();

	Gdk::RGBA rgba = Gdk::RGBA(color);

	cr->set_source_rgba (rgba.get_red(), rgba.get_green(), rgba.get_blue(), rgba.get_alpha());

	Gdk::Point s1 = get_coordinate (m_engine.RANK_X (src) - 3, m_engine.RANK_Y (src) - 3);
	Gdk::Point s2 = get_coordinate (m_engine.RANK_X (dst) - 3, m_engine.RANK_Y (dst) - 3);

	cr->set_line_width (4.0);
	cr->move_to(s1.get_x (), s1.get_y ());
	cr->line_to(s2.get_x (), s2.get_y ());
	cr->stroke();
}
void Board::first_move()
{
	m_step = 0;
	m_engine.get_snapshot(m_step);

	/** 设置此步的注释*/
	/** set the comment of this  move*/
	std::string* str=m_engine.get_comment(m_step);
	if(str != NULL){
		parent.set_comment(*str);
	}
	else
		parent.set_comment(" ");

	CSound::play(SND_MOVE);
	queue_draw();
}

void Board::last_move()
{
	m_step = m_engine.how_step();
	m_engine.get_snapshot(m_step);

	/** 设置此步的注释*/
	/** set the comment of this  move*/
	std::string* str=m_engine.get_comment(m_step);
	if(str != NULL){
		parent.set_comment(*str);
	}
	else
		parent.set_comment(" ");

	CSound::play(SND_MOVE);
	queue_draw();
}
void Board::next_move()
{
	m_step++;
	int all_step = m_engine.how_step();
	if(m_step> all_step)
		m_step = all_step;
	m_engine.get_snapshot(m_step);

	/** 设置此步的注释*/
	/** set the comment of this  move*/
	std::string* str=m_engine.get_comment(m_step);
	if(str != NULL){
		parent.set_comment(*str);
	}
	else
		parent.set_comment(" ");

	CSound::play(SND_MOVE);
	queue_draw();
}
void Board::back_move()
{

	m_step--;
	if(m_step<0)
		m_step =0;
	m_engine.get_snapshot(m_step);
	DLOG("m_step = %d\n",m_step);


	/** 设置此步的注释*/
	/** set the comment of this  move*/
	std::string* str=m_engine.get_comment(m_step);
	if(str != NULL){
		parent.set_comment(*str);
	}
	else
		parent.set_comment(" ");

	CSound::play(SND_MOVE);
	queue_draw();

}

void Board::get_board_by_move(int f_step)
{
	//m_step = (num+1)*2;
	int all_step = m_engine.how_step();
	if(f_step> all_step)
		f_step = all_step;
	DLOG("m_step = %d\n",f_step);

	m_engine.get_snapshot(f_step);
	m_step = f_step;

	/** 设置此步的注释*/
	/** set the comment of this  move*/
	std::string* str=m_engine.get_comment(f_step);
	if(str != NULL){
		parent.set_comment(*str);
	}
	else
		parent.set_comment(" ");
	queue_draw();
}

int Board::try_move(int dst_x,int dst_y)
{

	int dst = m_engine.get_dst_xy(dst_x,dst_y,is_rev_board);
	int src = m_engine.get_chessman_xy(selected_chessman);
	int mv =  m_engine.get_move(src,dst);
	return try_move(mv);
}
int Board::try_move(int mv)
{
	int eat = m_engine.get_move_eat(mv);
	int dst=  m_engine.get_move_dst(mv);
	/** 对着法进行逻辑检测*/
	/** check the logic of the move */
	if(m_engine.make_move(mv)){
		/** 执行着法*/
		//m_engine.do_move(mv);
		/** 将着法中文表示加到treeview中*/
		/**  and the chinese moves to treeview */
		Glib::ustring mv_chin = m_engine.get_chinese_last_move();
		int num = m_engine.how_step();
		parent.add_step_line(num,mv_chin);
		if(eat)
			CSound::play(SND_EAT);
		else
			CSound::play(SND_MOVE);

		queue_draw();
		selected_chessman = m_engine.get_piece(dst);
		printf("move = %d finish move and redraw now\n",mv);
		selected_chessman=-1;
		std::string iccs_str=m_engine.move_to_iccs_str(mv);
		/** 对战时的处理*/
		if(is_fight_to_robot()){
			if(eat){
				moves_lines.clear();
				moves_lines =position_str+ m_engine.get_last_fen_from_snapshot()+std::string(" -- 0 1 ");
			}
			else{
				size_t pos = moves_lines.find("moves");
				if(pos == std::string::npos){
					moves_lines=moves_lines + " -- 0 1  moves "+iccs_str;
				}else{
					moves_lines=moves_lines + " "+iccs_str;
				}

			}
			/** then send the moves_lines to ucci engine(robot)*/
			std::cout<<"moves_lines = "<<moves_lines<<std::endl;
			m_robot.send_ctrl_command(moves_lines.c_str());
			m_robot.send_ctrl_command("\n");
			//user_player = 1-user_player;
			if(!is_human_player()){
				//Glib::ustring str_cmd="go time "+to_msec_ustring(black_time)+" increment 0\n";
				//Glib::ustring str_cmd=Glib::ustring("go depth ")+search_depth+"\n";
				char str_cmd[256];
				sprintf(str_cmd,"go depth %d \n",m_search_depth);
				m_robot.send_ctrl_command(str_cmd);
				//m_robot.send_ctrl_command("go time 295000 increment 0\n");
			}


			parent.change_play(is_human_player());
			count_time=0;
		}
		else if(is_network_game()){

			if(!is_human_player()){
			/** 我走的棋，则将iccs_str走法传给网络*/
				printf("my go\n");
				std::string mv_str = "moves:"+iccs_str;
				send_to_socket(mv_str);


			}
			parent.change_play(is_human_player());
			count_time=0;

		}
		parent.set_comment("");

		/**被将死了*/
		/** is it  mate */
		if(m_engine.mate() && is_human_player() ){
			if(timer.connected())
				timer.disconnect();
			CSound::play(SND_CHECK);
			parent.on_end_game(ROBOT_WIN);
			parent.set_comment("不要气馁，再接再大励吧!");
			DLOG("将军死棋\n");
			return 0;
		}
		if(m_engine.get_checkby()){
			CSound::play(SND_CHECK);
			DLOG("将军===============\n");
			if(is_human_player())
				parent.set_comment("您被将军了，小心噢!");
			else
				parent.set_comment("将军，干得好，看好你噢!");
		}

	}
	return 0;

}

// 和棋
void Board::draw_move()
{

	if(is_fight_to_robot()){

		if(is_human_player())
			m_robot.send_ctrl_command("go draw\n");
		else
			m_robot.send_ctrl_command("ponderhit draw\n");

	}
	else if(is_network_game()){
		parent.on_end_game(ROBOT_DRAW);
	}

}

void Board::rue_move()
{

	if(m_engine.how_step()<1)
		return;
	DLOG(" how_step %d\n",m_engine.how_step());
	int mv = m_engine.get_last_move_from_snapshot();
	m_engine.undo_move(mv);
	parent.del_step_last_line();

	queue_draw();

	if(is_fight_to_robot()){
		moves_lines.clear();
		moves_lines =position_str+ m_engine.get_last_fen_from_snapshot()+std::string(" -- 0 1 ");
		m_robot.send_ctrl_command(moves_lines.c_str());
		m_robot.send_ctrl_command("\n");
	}
}


int Board::open_file(const std::string& filename)
{

	/** for test pgnfile */
	if(p_pgnfile->read(filename)<0){
		/** open fail */
		return -1;
	}
	m_step = m_engine.how_step();
	m_status = READ_STATUS ;

	queue_draw();
	return 0;
}


void Board::on_drog_data_received(const Glib::RefPtr<Gdk::DragContext>& context,
		int, int, const Gtk::SelectionData& selection_data,
		guint,guint f_time)
{
	if((selection_data.get_length() >= 0)&&(selection_data.get_format()== 8))
	{
		context->drag_finish(false,false,f_time);
		std::string filename = wind_unescape_string(selection_data.get_text().c_str(), NULL);
		size_t pos = filename.find('\r');
		if (std::string::npos != pos)
			filename = filename.substr(7, pos-7);
		parent.open_file(filename);

	}
}

void Board::free_game(bool redraw_)
{
	if(timer.connected())
		timer.disconnect();

	m_robot.send_ctrl_command("quit\n");
	m_robot.stop();
	m_status = FREE_STATUS;
	is_rev_board=false;
	m_human_black=false;
	if(redraw_){
		m_engine.init_snapshot(start_fen);
		queue_draw();
	}
}
void Board::rev_game()
{
	is_rev_board=!is_rev_board;
	m_human_black=!m_human_black;
	queue_draw();
}

void Board::start_robot(bool new_)
{
	m_status = FIGHT_STATUS;

	m_robot.set_engine(engine_name);
	m_robot.start();
	m_robot.send_ctrl_command("ucci\n");
	if(new_)
		new_game(m_status);
	else
		chanju_game(m_status);
}

void Board::set_level_config(int _depth,int _idle,int _style,int _knowledge,int _pruning,int _randomness,bool _usebook)
{
	m_search_depth = _depth;
	m_usebook = _usebook;

}

void Board::set_time(int _step_time, int _play_time)
{
	step_time = _step_time;
	play_time = _play_time;
}

void Board::set_war_time(int _step_time,int _play_time)
{
	limit_count_time = _step_time;
	red_time = _play_time*60;
	black_time = _play_time* 60;

}

void Board::set_level()
{

	/** test simple*/
	m_robot.send_ctrl_command("setoption idle large\n");
	m_robot.send_ctrl_command("setoption style risky\n");
	m_robot.send_ctrl_command("setoption knowledge none\n");
	m_robot.send_ctrl_command("setoption pruning  large\n");
	m_robot.send_ctrl_command("setoption randomness large\n");
	if(m_usebook)
		m_robot.send_ctrl_command("setoption usebook false\n");
	else
		m_robot.send_ctrl_command("setoption usebook true\n");
}

void Board::start_network()
{
	set_war_time(300,30);
	new_game(NETWORK_STATUS);

}

void Board::chanju_game(BOARD_STATUS _status)
{
	m_status = _status;
	std::string cur_fen = m_engine.get_current_snapshot();
	m_engine.init_snapshot(cur_fen.c_str());

	if(m_status == FIGHT_STATUS){
		set_war_time(step_time,play_time);
		m_robot.send_ctrl_command("setoption newgame\n");
		set_level();
	}
	DLOG("current fen = %s\n", cur_fen.c_str());


	moves_lines.clear();
	moves_lines = position_str + cur_fen;
	queue_draw();

	parent.textview_engine_log_clear();
	parent.change_play(is_human_player());

	timer=Glib::signal_timeout().connect(sigc::mem_fun(*this,&Board::go_time),1000);
	/**如果是用户选择黑方，则电脑先走棋 -- if user choose black,the robot go moves first*/
	if(m_human_black){
		if(m_status == FIGHT_STATUS){
			moves_lines =moves_lines +std::string(" -- 0 1 ");
			m_robot.send_ctrl_command(moves_lines.c_str());
			m_robot.send_ctrl_command("\n");
			char str_cmd[256];
			sprintf(str_cmd,"go depth %d \n",m_search_depth);
			m_robot.send_ctrl_command(str_cmd);
		}else if(m_status == NETWORK_STATUS){

		}

	}

	parent.set_red_war_time(to_time_ustring(red_time),to_time_ustring(0));
	parent.set_black_war_time(to_time_ustring(black_time),to_time_ustring(0));


}


void Board::new_game(BOARD_STATUS _status)
{
	m_status = _status;

	m_engine.init_snapshot(start_fen);

	if(m_status == FIGHT_STATUS){
		set_war_time(step_time,play_time);
		m_robot.send_ctrl_command("setoption newgame\n");
		set_level();
	}


	moves_lines.clear();
	moves_lines = position_str + std::string(start_fen);
	queue_draw();

	parent.textview_engine_log_clear();
	parent.change_play(is_human_player());

	timer=Glib::signal_timeout().connect(sigc::mem_fun(*this,&Board::go_time),1000);
	/**如果是用户选择黑方，则电脑先走棋 -- if user choose black,the robot go moves first*/
	if(m_human_black){
		if(m_status == FIGHT_STATUS){
			moves_lines =moves_lines +std::string(" -- 0 1 ");
			m_robot.send_ctrl_command(moves_lines.c_str());
			m_robot.send_ctrl_command("\n");
			char str_cmd[256];
			sprintf(str_cmd,"go depth %d \n",m_search_depth);
			m_robot.send_ctrl_command(str_cmd);
		}else if(m_status == NETWORK_STATUS){

		}

	}

	parent.set_red_war_time(to_time_ustring(red_time),to_time_ustring(0));
	parent.set_black_war_time(to_time_ustring(black_time),to_time_ustring(0));
}


bool Board::robot_log(const Glib::IOCondition& condition)
{
	/*for testing,delete me*/
	char buf[1024] = { 0 };
	int buf_len = 1024;
	char* p = buf;
	for (; buf_len > 0; ) {
		int len = m_robot.get_robot_log(p, buf_len);
		if (len <= 0)
			break;
		p += len;
		buf_len -= len;
	}

	if (buf_len > 0) {
		*p = 0;
		printf ("%s", buf);
		std::string str_buf(buf);
		parent.show_textview_engine_log(str_buf);

		size_t pos_=str_buf.find("draw");
		if(pos_ != std::string::npos){

			printf("计算机同意和棋\n");
			if (parent.on_end_game(ROBOT_DRAW)) {
				if(timer.connected())
					timer.disconnect();
				return 1;
			}
		}
		pos_=str_buf.find("resign");
		if(pos_ != std::string::npos){

			if(timer.connected())
				timer.disconnect();
			parent.on_end_game(ROBOT_LOSE);
			return true;
		}
		pos_=str_buf.find("nobestmove");
		if(pos_ != std::string::npos){
			if(timer.connected())
				timer.disconnect();
			parent.on_end_game(ROBOT_LOSE);
			return true;
		}
		size_t pos=str_buf.find("bestmove");
		if(pos != std::string::npos){
			std::string t_mv=str_buf.substr(pos+9,4);
			std::cout<<"get robot mv = "<<t_mv<<std::endl;
			int mv = m_engine.iccs_str_to_move(t_mv);
			try_move(mv);
		}
	}

	return true;

}

Glib::ustring Board::to_time_ustring(int ival)
{
	char sp[32];
	sprintf(sp,"%02d:%02d", ival/60,ival%60);
	return Glib::ustring(sp);
}

Glib::ustring Board::to_msec_ustring(int ival)
{
	char sp[32];
	sprintf(sp,"%d000",ival);
	return Glib::ustring(sp);
}

bool Board::go_time()
{
	if(is_human_player()){
		count_time++;
		red_time--;
		parent.set_red_war_time(to_time_ustring(red_time),to_time_ustring(count_time));
		/** when the user 's step time over the limit time,we must warn it,and when over time the has not go,it will be losed */
		if(count_time>limit_count_time-10 && count_time<= limit_count_time){
			printf("time out,you less time: %d\n",limit_count_time-count_time);
			reckon_time_sound(limit_count_time-count_time);
		}
		else if(count_time > limit_count_time || red_time<0 ){
			printf(" time limit ,you lose\n");
			if(timer.connected())
				timer.disconnect();
			parent.on_end_game(HUMAN_OVER_TIME);
			count_time =0;
			if(is_network_game())
				send_to_socket("timeout");

		}
	}
	else{
		black_time--;
		count_time++;
		parent.set_black_war_time(to_time_ustring(black_time),to_time_ustring(count_time));
		//printf("black_time: %d\n",black_time);
		if(count_time>limit_count_time-10 && count_time<=limit_count_time){
			printf("time out,bot waster much time\n");
			m_robot.send_ctrl_command("stop\n");
		}
		else if(count_time >limit_count_time || black_time<0 ){
			printf(" time limit ,robot lose\n");
			if(timer.connected())
				timer.disconnect();
			parent.on_end_game(ROBOT_OVER_TIME);
			count_time =0;
			if(is_network_game())
				send_to_socket("timeout");
		}
	}


	return true;
}

void Board::set_board_size(BOARDSIZE sizemode)
{
	switch(sizemode){
		case BIG_BOARD:
			is_small_board=false;
			this->set_size_request(521,577);
			break;
		case SMALL_BOARD:
			is_small_board=true;
			this->set_size_request(221,277);
			break;
		default:
			g_warn_if_reached();
			break;
	}
}

void Board::reckon_time_sound(int time_)
{
	switch(time_){
		case 0:
			CSound::play(SND_0);
			break;
		case 1:
			CSound::play(SND_1);
			break;
		case 2:
			CSound::play(SND_2);
			break;
		case 3:
			CSound::play(SND_3);
			break;
		case 4:
			CSound::play(SND_4);
			break;
		case 5:
			CSound::play(SND_5);
			break;
		case 6:
			CSound::play(SND_6);
			break;
		case 7:
			CSound::play(SND_7);
			break;
		case 8:
			CSound::play(SND_8);
			break;
		case 9:
			CSound::play(SND_9);
			break;
		case 10:
			CSound::play(SND_10);
			break;
		default:
			break;

	}

}

void Board::watch_socket(int fd)
{
	fd_recv_skt=fd;
	Glib::signal_io().connect(sigc::mem_fun(*this,&Board::on_network_io),
			fd_recv_skt, Glib::IO_IN);

}
bool Board::on_network_io(const Glib::IOCondition& )
{

	int fd_cli = -1;
       EC_THROW(-1 == (fd_cli = accept(fd_recv_skt, NULL, 0)));
       char buf[1024];
       size_t len = read(fd_cli, &buf[0], 1023);
       buf[len]=0;
       if (len > 0) {

		std::string str_buf(buf);
		size_t pos;
		//size_t pos_=str_buf.find("network-game-red");

		if((pos = str_buf.find("moves:")) != std::string::npos){
			std::string t_mv=str_buf.substr(pos+6,4);
			std::cout<<"get robot mv = "<<t_mv<<std::endl;
			int mv = m_engine.iccs_str_to_move(t_mv);
			try_move(mv);

		}else if((pos = str_buf.find("network-game-red,"))!= std::string::npos){
			//start network game with red player
			std::string enemy_name,my_name;
			size_t pos_s,pos_e,pos_m;
			pos_s= str_buf.find("enemy_name:");
			pos_e= str_buf.find(",my_name:");
			pos_m= str_buf.find_first_of("@");
			enemy_name = str_buf.substr(pos_s+11,pos_m-pos_s-11);
			pos_m = str_buf.find_last_of("@");
			my_name = str_buf.substr(pos_e+9,pos_m-pos_e-9);

			//parent.on_network_game(my_name,enemy_name,true);
			parent.on_network_game(enemy_name,my_name,true);
		}else if((pos = str_buf.find("network-game-black,")) != std::string::npos){
			//start network game with black player
			std::string enemy_name,my_name;
			size_t pos_s,pos_e,pos_m;
			pos_s= str_buf.find("enemy_name:");
			pos_e= str_buf.find(",my_name:");
			pos_m= str_buf.find_first_of("@");
			enemy_name = str_buf.substr(pos_s+11,pos_m-pos_s-11);
			pos_m = str_buf.find_last_of("@");
			my_name = str_buf.substr(pos_e+9,pos_m-pos_e-9);
			parent.on_network_game(my_name,enemy_name,false);
		}else if(str_buf.find("network-game-win") != std::string::npos){
			if(timer.connected())
				timer.disconnect();
			parent.on_end_game(ROBOT_LOSE);
			return true;
		}else if(str_buf.find("resign") != std::string::npos){
			if(timer.connected())
				timer.disconnect();
			parent.on_end_game(ROBOT_WIN);
			return true;
		}else if(str_buf.find("network-game-rue") != std::string::npos){
			rue_move();
			rue_move();
		}else if(str_buf.find("network-game-norue") != std::string::npos){
			parent.info_window("The against doesn't agree rue!");
		}else if(str_buf.find("network-game-nodraw") != std::string::npos){
			parent.info_window(("The against doesn't agree draw!"));
		}else if(str_buf.find("network-game-draw") != std::string::npos){
			if(timer.connected())
				timer.disconnect();
			parent.on_end_game(ROBOT_DRAW);
		}else if(str_buf.find("enemy_name:") != std::string::npos){

		}else if(str_buf.find("my_name:") !=std::string::npos){

		}


	}
	close(fd_cli);
	return true;
}


int Board::init_send_socket()
{
	int sockfd;
	struct sockaddr_in srvaddr;

	EC_THROW(-1 == (sockfd=socket(AF_INET,SOCK_STREAM,0)));
	memset(&srvaddr,0,sizeof(srvaddr));
	srvaddr.sin_family=AF_INET;
	srvaddr.sin_port=htons(GMPORT+1);
	srvaddr.sin_addr.s_addr=htonl(INADDR_ANY);

	int on = 1;
	EC_THROW( -1 == (setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) )));

	return sockfd;
}
void Board::send_to_socket(const std::string& cmd_)
{

	int sockfd;
	struct sockaddr_in srvaddr;

	EC_THROW(-1 == (sockfd=socket(AF_INET,SOCK_STREAM,0)));
	memset(&srvaddr,0,sizeof(srvaddr));
	srvaddr.sin_family=AF_INET;
	srvaddr.sin_port=htons(GMPORT+1);
	srvaddr.sin_addr.s_addr=htonl(INADDR_ANY);

	int on = 1;
	EC_THROW( -1 == (setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) )));

	if( 0 == connect(sockfd,(struct sockaddr*)&srvaddr,sizeof(srvaddr))){
		write(sockfd,cmd_.c_str(),cmd_.size());
		close(sockfd);
	}
}
void Board::close_send_socket()
{
	if(fd_send_skt>0){
		close(fd_send_skt);
		fd_send_skt=-1;
	}
}

void Board::save_board_to_file(const std::string& filename)
{
	Glib::RefPtr<Gdk::Window> window;
	Cairo::RefPtr<Cairo::Context> src_cr;
	Cairo::RefPtr<Cairo::Context> dst_cr;
	Cairo::RefPtr<Cairo::Surface> src_surface;
	Cairo::RefPtr<Cairo::ImageSurface> dst_surface;

	window = get_window ();
	src_cr = window->create_cairo_context ();
	src_surface = src_cr->get_target();

	dst_surface = Cairo::ImageSurface::create(Cairo::Format::FORMAT_ARGB32,
						  get_allocated_width(),
						  get_allocated_height());
	dst_cr = Cairo::Context::create(dst_surface);
	dst_cr->set_source(src_surface, 0, 0);
	dst_cr->paint();
	dst_surface->write_to_png (filename);
}
