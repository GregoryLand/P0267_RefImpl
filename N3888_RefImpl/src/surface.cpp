#include "io2d.h"
#include "xio2dhelpers.h"
#include "xcairoenumhelpers.h"
#include <algorithm>

using namespace std;
using namespace std::experimental::io2d;

surface::native_handle_type surface::native_handle() const {
	return{ _Surface.get(), _Context.get() };
}

surface::surface(surface::native_handle_type nh)
	: _Lock_for_device()
	, _Device()
	, _Surface(unique_ptr<cairo_surface_t, function<void(cairo_surface_t*)>>(nh.csfce, &cairo_surface_destroy))
	, _Context(unique_ptr<cairo_t, function<void(cairo_t*)>>(((nh.csfce == nullptr) ? nullptr : cairo_create(nh.csfce)), &cairo_destroy)) {
	if (_Context.get() != nullptr) {
		cairo_set_miter_limit(_Context.get(), _Line_join_miter_miter_limit);
	}
}

surface::surface(surface&& other)
	: _Lock_for_device()
	, _Device(move(other._Device))
	, _Surface(move(other._Surface))
	, _Context(move(other._Context)) {
	other._Surface = nullptr;
	other._Context = nullptr;
}

surface& surface::operator=(surface&& other) {
	if (this != &other) {
		_Device = move(other._Device);
		_Surface = move(other._Surface);
		_Context = move(other._Context);
		other._Surface = nullptr;
		other._Context = nullptr;
	}
	return *this;
}

surface::surface(const surface& other, content content, int width, int height)
	: _Lock_for_device()
	, _Device()
	, _Surface(unique_ptr<cairo_surface_t, function<void(cairo_surface_t*)>>(cairo_surface_create_similar(other._Surface.get(), _Content_to_cairo_content_t(content), width, height), &cairo_surface_destroy))
	, _Context(unique_ptr<cairo_t, function<void(cairo_t*)>>(cairo_create(_Surface.get()), &cairo_destroy)) {
}

surface::surface(format fmt, int width, int height)
	: _Lock_for_device()
	, _Device()
	, _Surface(unique_ptr<cairo_surface_t, function<void(cairo_surface_t*)>>(cairo_image_surface_create(_Format_to_cairo_format_t(fmt), width, height), &cairo_surface_destroy))
	, _Context(unique_ptr<cairo_t, function<void(cairo_t*)>>(cairo_create(_Surface.get()), &cairo_destroy)) {
}

surface::~surface() {
}

void surface::finish() {
	cairo_surface_finish(_Surface.get());
}

void surface::flush() {
	cairo_surface_flush(_Surface.get());
}

shared_ptr<device> surface::get_device() {
	lock_guard<mutex> lg(_Lock_for_device);
	auto dvc = _Device.lock();
	if (dvc != nullptr) {
		return dvc;
	}
	dvc = shared_ptr<device>(new device(cairo_surface_get_device(_Surface.get())));
	_Device = weak_ptr<device>(dvc);
	return dvc;
}

content surface::get_content() const {
	return _Cairo_content_t_to_content(cairo_surface_get_content(_Surface.get()));
}

void surface::mark_dirty() {
	cairo_surface_mark_dirty(_Surface.get());
}

void surface::mark_dirty(const rectangle& rect) {
	cairo_surface_mark_dirty_rectangle(_Surface.get(), _Double_to_int(rect.x(), false), _Double_to_int(rect.y(), false), _Double_to_int(rect.width()), _Double_to_int(rect.height()));
}

void surface::set_device_offset(const point& offset) {
	cairo_surface_set_device_offset(_Surface.get(), offset.x(), offset.y());
}

point surface::get_device_offset() const {
	double x, y;
	cairo_surface_get_device_offset(_Surface.get(), &x, &y);
	return point{ x, y };
}

void surface::write_to_file(const string& filename) {
	_Throw_if_failed_cairo_status_t(cairo_surface_write_to_png(_Surface.get(), filename.c_str()));
}

image_surface surface::map_to_image() {
	return image_surface({ cairo_surface_map_to_image(_Surface.get(), nullptr), nullptr }, { _Surface.get(), _Context.get() });
}

image_surface surface::map_to_image(const rectangle& extents) {
	cairo_rectangle_int_t cextents{ _Double_to_int(extents.x()), _Double_to_int(extents.y()), _Double_to_int(extents.width()), _Double_to_int(extents.height()) };

	return image_surface({ cairo_surface_map_to_image(_Surface.get(), (extents.x() == 0 && extents.y() == 0 && extents.width() == 0 && extents.height() == 0) ? nullptr : &cextents), nullptr }, { _Surface.get(), _Context.get() });
}

void surface::unmap_image(image_surface& image) {
	image._Context = nullptr;
	image._Surface = nullptr;
}

bool surface::has_surface_resource() const {
	return _Surface != nullptr;
}

void surface::save() {
	cairo_save(_Context.get());
}

void surface::restore() {
	cairo_restore(_Context.get());
}

void surface::set_pattern() {
	cairo_set_source_rgb(_Context.get(), 0.0, 0.0, 0.0);
}

void surface::set_pattern(const pattern& source) {
	cairo_set_source(_Context.get(), source.native_handle());
}

pattern surface::get_pattern() const {
	return pattern(cairo_pattern_reference(cairo_get_source(_Context.get())));
}

void surface::set_antialias(antialias a) {
	cairo_set_antialias(_Context.get(), _Antialias_to_cairo_antialias_t(a));
}

antialias surface::get_antialias() const {
	return _Cairo_antialias_t_to_antialias(cairo_get_antialias(_Context.get()));
}

void surface::set_dashes() {
	cairo_set_dash(_Context.get(), nullptr, 0, 0.0);
}

void surface::set_dashes(const dashes& d) {
	cairo_set_dash(_Context.get(), get<0>(d).data(), _Container_size_to_int(get<0>(d)), get<1>(d));
}

int surface::get_dashes_count() const {
	return cairo_get_dash_count(_Context.get());
}

surface::dashes surface::get_dashes() const {
	dashes result{ };
	auto& d = get<0>(result);
	auto& o = get<1>(result);
	d.resize(get_dashes_count());
	cairo_get_dash(_Context.get(), &d[0], &o);
	return result;
}

void surface::set_fill_rule(fill_rule fr) {
	cairo_set_fill_rule(_Context.get(), _Fill_rule_to_cairo_fill_rule_t(fr));
}

fill_rule surface::get_fill_rule() const {
	return _Cairo_fill_rule_t_to_fill_rule(cairo_get_fill_rule(_Context.get()));
}

void surface::set_line_cap(line_cap lc) {
	cairo_set_line_cap(_Context.get(), _Line_cap_to_cairo_line_cap_t(lc));
}

line_cap surface::get_line_cap() const {
	return _Cairo_line_cap_t_to_line_cap(cairo_get_line_cap(_Context.get()));
}

void surface::set_line_join(line_join lj) {
	_Line_join = lj;
	cairo_set_line_join(_Context.get(), _Line_join_to_cairo_line_join_t(lj));
	if (lj == line_join::miter_or_bevel) {
		cairo_set_miter_limit(_Context.get(), _Miter_limit);
	}
	if (lj == line_join::miter) {
		cairo_set_miter_limit(_Context.get(), _Line_join_miter_miter_limit);
	}
}

line_join surface::get_line_join() const {
	return _Line_join;
}

void surface::set_line_width(double width) {
	cairo_set_line_width(_Context.get(), std::max(width, 0.0));
}

double surface::get_line_width() const {
	return cairo_get_line_width(_Context.get());
}

void surface::set_miter_limit(double limit) {
	_Miter_limit = limit;
	if (_Line_join == line_join::miter_or_bevel) {
		cairo_set_miter_limit(_Context.get(), std::min(std::max(limit, 1.0), _Line_join_miter_miter_limit));
	}
}

double surface::get_miter_limit() const {
	return _Miter_limit;
}

void surface::set_compositing_operator(compositing_operator co) {
	cairo_set_operator(_Context.get(), _Compositing_operator_to_cairo_operator_t(co));
}

compositing_operator surface::get_compositing_operator() const {
	return _Cairo_operator_t_to_compositing_operator(cairo_get_operator(_Context.get()));
}

void surface::set_tolerance(double tolerance) {
	cairo_set_tolerance(_Context.get(), tolerance);
}

double surface::get_tolerance() const {
	return cairo_get_tolerance(_Context.get());
}

void surface::clip() {
	cairo_clip_preserve(_Context.get());
}

rectangle surface::get_clip_extents() const {
	double pt0x, pt0y, pt1x, pt1y;
	cairo_clip_extents(_Context.get(), &pt0x, &pt0y, &pt1x, &pt1y);
	return{ min(pt0x, pt1x), min(pt0y, pt1y), max(pt0x, pt1x) - min(pt0x, pt1x), max(pt0y, pt1y) - min(pt0y, pt1y) };
}

bool surface::in_clip(const point& pt) const {
	return cairo_in_clip(_Context.get(), pt.x(), pt.y()) != 0;
}

void surface::reset_clip() {
	cairo_reset_clip(_Context.get());
}

vector<rectangle> surface::get_clip_rectangles() const {
	vector<rectangle> results;
	unique_ptr<cairo_rectangle_list_t, function<void(cairo_rectangle_list_t*)>> sp_rects(cairo_copy_clip_rectangle_list(_Context.get()), &cairo_rectangle_list_destroy);
	_Throw_if_failed_cairo_status_t(sp_rects->status);
	for (auto i = 0; i < sp_rects->num_rectangles; ++i) {
		results.push_back({ sp_rects->rectangles[i].x, sp_rects->rectangles[i].y, sp_rects->rectangles[i].width, sp_rects->rectangles[i].height });
	}

	return results;
}

void surface::fill() {
	cairo_fill_preserve(_Context.get());
}

void surface::fill(const surface& s) {
	unique_ptr<cairo_pattern_t, function<void(cairo_pattern_t*)>> pat(cairo_pattern_reference(cairo_get_source(_Context.get())), &cairo_pattern_destroy);
	cairo_set_source_surface(_Context.get(), s.native_handle().csfce, 0.0, 0.0);
	cairo_fill_preserve(_Context.get());
	cairo_surface_flush(_Surface.get());
	cairo_set_source(_Context.get(), pat.get());
}

rectangle surface::get_fill_extents() const {
	double pt0x, pt0y, pt1x, pt1y;
	cairo_fill_extents(_Context.get(), &pt0x, &pt0y, &pt1x, &pt1y);
	return{ min(pt0x, pt1x), min(pt0y, pt1y), max(pt0x, pt1x) - min(pt0x, pt1x), max(pt0y, pt1y) - min(pt0y, pt1y) };
}

bool surface::in_fill(const point& pt) const {
	return cairo_in_fill(_Context.get(), pt.x(), pt.y()) != 0;
}

void surface::mask(const pattern& pttn) {
	cairo_mask(_Context.get(), pttn.native_handle());
}

void surface::mask(const surface& surface) {
	cairo_mask_surface(_Context.get(), surface.native_handle().csfce, 0.0, 0.0);
}

void surface::mask(const surface& surface, const point& origin) {
	cairo_mask_surface(_Context.get(), surface.native_handle().csfce, origin.x(), origin.y());
}

void surface::paint() {
	cairo_paint(_Context.get());
}

void surface::paint(const surface& s) {
	unique_ptr<cairo_pattern_t, function<void(cairo_pattern_t*)>> pat(cairo_pattern_reference(cairo_get_source(_Context.get())), &cairo_pattern_destroy);
	cairo_set_source_surface(_Context.get(), s.native_handle().csfce, 0.0, 0.0);
	cairo_paint(_Context.get());
	cairo_surface_flush(_Surface.get());
	cairo_set_source(_Context.get(), pat.get());
}

void surface::paint(double alpha) {
	cairo_paint_with_alpha(_Context.get(), alpha);
}

void surface::paint(const surface& s, double alpha) {
	auto pat = cairo_get_source(_Context.get());
	cairo_set_source_surface(_Context.get(), s.native_handle().csfce, 0.0, 0.0);
	cairo_paint_with_alpha(_Context.get(), alpha);
	cairo_surface_flush(_Surface.get());
	cairo_set_source(_Context.get(), pat);
}

void surface::stroke() {
	cairo_stroke_preserve(_Context.get());
}

void surface::stroke(const surface& s) {
	unique_ptr<cairo_pattern_t, function<void(cairo_pattern_t*)>> pat(cairo_pattern_reference(cairo_get_source(_Context.get())), &cairo_pattern_destroy);
	cairo_set_source_surface(_Context.get(), s.native_handle().csfce, 0.0, 0.0);
	cairo_stroke_preserve(_Context.get());
	cairo_surface_flush(_Surface.get());
	cairo_set_source(_Context.get(), pat.get());
}

rectangle surface::get_stroke_extents() const {
	double pt0x, pt0y, pt1x, pt1y;
	cairo_stroke_extents(_Context.get(), &pt0x, &pt0y, &pt1x, &pt1y);
	return{ min(pt0x, pt1x), min(pt0y, pt1y), max(pt0x, pt1x) - min(pt0x, pt1x), max(pt0y, pt1y) - min(pt0y, pt1y) };
}

bool surface::in_stroke(const point& pt) const {
	return cairo_in_stroke(_Context.get(), pt.x(), pt.y()) != 0;
}

void surface::set_path() {
	cairo_new_path(_Context.get());
}

point _Rotate_point_absolute_angle(const point& center, double radius, double angle) {
	point pt{ radius * cos(angle), -radius * -sin(angle) };
	pt += center;
	return pt;
}

void surface::set_path(const path& p) {
	auto ctx = _Context.get();
	auto matrix = matrix_2d::init_identity();
	point origin{ };
	bool hasCurrentPoint = false;
	point currentPoint{ };
	cairo_new_path(ctx);
	const auto& pathData = p.get_data_ref();
	for (unsigned int i = 0; i < pathData.size(); i++) {
		const auto& item = pathData[i];
		auto pdt = item->type();
		switch (pdt) {
		case std::experimental::io2d::path_data_type::move_to:
		{
			currentPoint = dynamic_cast<move_to_path_data*>(item.get())->to();
			auto pt = matrix.transform_point(currentPoint - origin) + origin;
			cairo_move_to(ctx, pt.x(), pt.y());
			hasCurrentPoint = true;
		} break;
		case std::experimental::io2d::path_data_type::line_to:
		{
			currentPoint = dynamic_cast<line_to_path_data*>(item.get())->to();
			auto pt = matrix.transform_point(currentPoint - origin) + origin;
			cairo_line_to(ctx, pt.x(), pt.y());
			hasCurrentPoint = true;
		} break;
		case std::experimental::io2d::path_data_type::curve_to:
		{
			auto dataItem = dynamic_cast<curve_to_path_data*>(item.get());
			auto pt1 = matrix.transform_point(dataItem->control_point_1() - origin) + origin;
			auto pt2 = matrix.transform_point(dataItem->control_point_2() - origin) + origin;
			auto pt3 = matrix.transform_point(dataItem->end_point() - origin) + origin;
			cairo_curve_to(ctx, pt1.x(), pt1.y(), pt2.x(), pt2.y(), pt3.x(), pt3.y());
			currentPoint = dataItem->end_point();
			hasCurrentPoint = true;
		} break;
		case std::experimental::io2d::path_data_type::new_sub_path:
		{
			cairo_new_sub_path(ctx);
			hasCurrentPoint = false;
		} break;
		case std::experimental::io2d::path_data_type::close_path:
		{
			cairo_close_path(ctx);
			hasCurrentPoint = false;
		} break;
		case std::experimental::io2d::path_data_type::rel_move_to:
		{
			assert(hasCurrentPoint);
			auto dataItem = dynamic_cast<rel_move_to_path_data*>(item.get());
			auto pt = matrix.transform_point((dataItem->to() + currentPoint) - origin) + origin;
			cairo_move_to(ctx, pt.x(), pt.y());
			currentPoint = dataItem->to() + currentPoint;
			hasCurrentPoint = true;
		} break;
		case std::experimental::io2d::path_data_type::rel_line_to:
		{
			assert(hasCurrentPoint);
			auto dataItem = dynamic_cast<rel_line_to_path_data*>(item.get());
			auto pt = matrix.transform_point((dataItem->to() + currentPoint) - origin) + origin;
			cairo_line_to(ctx, pt.x(), pt.y());
			currentPoint = dataItem->to() + currentPoint;
			hasCurrentPoint = true;
		} break;
		case std::experimental::io2d::path_data_type::rel_curve_to:
		{
			assert(hasCurrentPoint);
			auto dataItem = dynamic_cast<curve_to_path_data*>(item.get());
			auto pt1 = matrix.transform_point((dataItem->control_point_1() + currentPoint) - origin) + origin;
			auto pt2 = matrix.transform_point((dataItem->control_point_2() + currentPoint) - origin) + origin;
			auto pt3 = matrix.transform_point((dataItem->end_point() + currentPoint) - origin) + origin;
			cairo_curve_to(ctx, pt1.x(), pt1.y(), pt2.x(), pt2.y(), pt3.x(), pt3.y());
			currentPoint = dataItem->end_point() + currentPoint;
			hasCurrentPoint = true;
		} break;
		case std::experimental::io2d::path_data_type::arc:
		{
			auto dataItem = dynamic_cast<arc_path_data*>(item.get());
			auto data = _Get_arc_as_beziers(dataItem->center(), dataItem->radius(), dataItem->angle_1(), dataItem->angle_2(), false, hasCurrentPoint, currentPoint, origin, matrix);
			for (const auto& arcItem : data) {
				switch (arcItem->type()) {
				case std::experimental::io2d::path_data_type::move_to:
				{
					auto pt = matrix.transform_point(dynamic_cast<move_to_path_data*>(arcItem.get())->to() - origin) + origin;
					cairo_move_to(ctx, pt.x(), pt.y());
				} break;
				case std::experimental::io2d::path_data_type::line_to:
				{
					auto pt = matrix.transform_point(dynamic_cast<line_to_path_data*>(arcItem.get())->to() - origin) + origin;
					cairo_line_to(ctx, pt.x(), pt.y());
				} break;
				case std::experimental::io2d::path_data_type::curve_to:
				{
					auto arcDataItem = dynamic_cast<curve_to_path_data*>(arcItem.get());
					auto pt1 = matrix.transform_point(arcDataItem->control_point_1() - origin) + origin;
					auto pt2 = matrix.transform_point(arcDataItem->control_point_2() - origin) + origin;
					auto pt3 = matrix.transform_point(arcDataItem->end_point() - origin) + origin;
					cairo_curve_to(ctx, pt1.x(), pt1.y(), pt2.x(), pt2.y(), pt3.x(), pt3.y());
				} break;
				case path_data_type::change_origin:
				{
					// Ignore, it's just spitting out the value we handed it.
				} break;
				case path_data_type::change_matrix:
				{
					// Ignore, it's just spitting out the value we handed it.
				} break;
				case path_data_type::new_sub_path:
				{
					cairo_new_sub_path(ctx);
				} break;
				default:
					assert("Unexpected path_data_type in arc." && false);
					break;
				}
			}
			currentPoint = _Rotate_point_absolute_angle(dataItem->center(), dataItem->radius(), dataItem->angle_2());
			hasCurrentPoint = true;
		}
			break;
		case std::experimental::io2d::path_data_type::arc_negative:
		{
			auto dataItem = dynamic_cast<arc_negative_path_data*>(item.get());
			auto data = _Get_arc_as_beziers(dataItem->center(), dataItem->radius(), dataItem->angle_1(), dataItem->angle_2(), true, hasCurrentPoint, currentPoint, origin, matrix);
			for (const auto& arcItem : data) {
				switch (arcItem->type()) {
				case std::experimental::io2d::path_data_type::move_to:
				{
					auto pt = matrix.transform_point(dynamic_cast<move_to_path_data*>(arcItem.get())->to() - origin) + origin;
					cairo_move_to(ctx, pt.x(), pt.y());
				} break;
				case std::experimental::io2d::path_data_type::line_to:
				{
					auto pt = matrix.transform_point(dynamic_cast<line_to_path_data*>(arcItem.get())->to() - origin) + origin;
					cairo_line_to(ctx, pt.x(), pt.y());
				} break;
				case std::experimental::io2d::path_data_type::curve_to:
				{
					auto arcDataItem = dynamic_cast<curve_to_path_data*>(arcItem.get());
					auto pt1 = matrix.transform_point(arcDataItem->control_point_1() - origin) + origin;
					auto pt2 = matrix.transform_point(arcDataItem->control_point_2() - origin) + origin;
					auto pt3 = matrix.transform_point(arcDataItem->end_point() - origin) + origin;
					cairo_curve_to(ctx, pt1.x(), pt1.y(), pt2.x(), pt2.y(), pt3.x(), pt3.y());
				} break;
				case path_data_type::change_origin:
				{
					// Ignore, it's just spitting out the value we handed it.
				} break;
				case path_data_type::change_matrix:
				{
					// Ignore, it's just spitting out the value we handed it.
				} break;
				case path_data_type::new_sub_path:
				{
					cairo_new_sub_path(ctx);
				} break;
				default:
					assert("Unexpected path_data_type in arc." && false);
					break;
				}
			}
			currentPoint = _Rotate_point_absolute_angle(dataItem->center(), dataItem->radius(), dataItem->angle_2());
			hasCurrentPoint = true;
		}
			break;
		case std::experimental::io2d::path_data_type::change_matrix:
		{
			matrix = dynamic_cast<change_matrix_path_data*>(item.get())->matrix();
		} break;
		case std::experimental::io2d::path_data_type::change_origin:
		{
			origin = dynamic_cast<change_origin_path_data*>(item.get())->origin();
		} break;
		default:
		{
			_Throw_if_failed_cairo_status_t(CAIRO_STATUS_INVALID_PATH_DATA);
		} break;
		}
	}
}

void surface::set_matrix(const matrix_2d& m) {
	cairo_matrix_t cm{ m.m00(), m.m01(), m.m10(), m.m11(), m.m20(), m.m21() };
	cairo_set_matrix(_Context.get(), &cm);
}

matrix_2d surface::get_matrix() const {
	cairo_matrix_t cm{ };
	cairo_get_matrix(_Context.get(), &cm);
	return{ cm.xx, cm.yx, cm.xy, cm.yy, cm.x0, cm.y0 };
}

point surface::user_to_device() const {
	double x, y;
	cairo_user_to_device(_Context.get(), &x, &y);
	return point{ x, y };
}

point surface::user_to_device_distance() const {
	double x, y;
	cairo_user_to_device_distance(_Context.get(), &x, &y);
	return point{ x, y };
}

point surface::device_to_user() const {
	double x, y;
	cairo_device_to_user(_Context.get(), &x, &y);
	return point{ x, y };
}

point surface::device_to_user_distance() const {
	double x, y;
	cairo_device_to_user_distance(_Context.get(), &x, &y);
	return point{ x, y };
}

void surface::select_font_face(const string& family, font_slant slant, font_weight weight) {
	cairo_select_font_face(_Context.get(), family.c_str(), _Font_slant_to_cairo_font_slant_t(slant), _Font_weight_to_cairo_font_weight_t(weight));
}

void surface::set_font_size(double size) {
	cairo_set_font_size(_Context.get(), size);
}

void surface::set_font_matrix(const matrix_2d& m) {
	cairo_matrix_t cm{ m.m00(), m.m01(), m.m10(), m.m11(), m.m20(), m.m21() };
	cairo_set_font_matrix(_Context.get(), &cm);
}

matrix_2d surface::get_font_matrix() const {
	cairo_matrix_t cm{ };
	cairo_get_font_matrix(_Context.get(), &cm);
	return{ cm.xx, cm.yx, cm.xy, cm.yy, cm.x0, cm.y0 };
}

void surface::set_font_options(const font_options& options) {
	cairo_set_font_options(_Context.get(), options.native_handle());
}

// Note: This deviates from cairo in that we return the values that will actually wind up being used.
font_options surface::get_font_options() const {
	font_options fo(antialias::default_antialias, subpixel_order::default_subpixel_order);
	cairo_get_font_options(_Context.get(), fo.native_handle());
	auto ca = fo.get_antialias();
	auto cso = fo.get_subpixel_order();
	cairo_surface_get_font_options(_Surface.get(), fo.native_handle());

	return font_options(
		(ca == antialias::default_antialias) ? fo.get_antialias() : ca,
		(cso == subpixel_order::default_subpixel_order) ? fo.get_subpixel_order() : cso
		);
}

void surface::set_font_face(const font_face& font_face) {
	cairo_set_font_face(_Context.get(), font_face.native_handle());
}

font_face surface::get_font_face() const {
	auto ff = cairo_get_font_face(_Context.get());
	_Throw_if_failed_cairo_status_t(cairo_font_face_status(ff));
	// Cairo doesn't increase the font face's reference count when you call cairo_get_font_face so we do it manually.
	return font_face(cairo_font_face_reference(ff));
}

void surface::set_scaled_font(const scaled_font& scaled_font) {
	cairo_set_scaled_font(_Context.get(), scaled_font.native_handle());
}

scaled_font surface::get_scaled_font() const {
	auto sf = cairo_get_scaled_font(_Context.get());
	_Throw_if_failed_cairo_status_t(cairo_scaled_font_status(sf));
	// Cairo doesn't increase the scaled font's reference count when you call cairo_get_scaled_font so we do it manually.
	return scaled_font(cairo_scaled_font_reference(sf));
}

void surface::show_text(const string& utf8) {
	cairo_show_text(_Context.get(), utf8.c_str());
}

void surface::show_glyphs(const vector<glyph>& glyphs) {
	vector<cairo_glyph_t> vec;
	for (const auto& glyph : glyphs) {
		vec.push_back({ glyph.index, glyph.x, glyph.y });
	}
	cairo_show_glyphs(_Context.get(), vec.data(), _Container_size_to_int(vec));
}

void surface::show_text_glyphs(const string& utf8, const vector<glyph>& glyphs, const vector<text_cluster>& clusters, bool clusterToGlyphsMapReverse) {
	vector<cairo_glyph_t> vec;
	for (const auto& glyph : glyphs) {
		vec.push_back({ glyph.index, glyph.x, glyph.y });
	}
	const auto tcSize = _Container_size_to_int(clusters);
	unique_ptr<cairo_text_cluster_t, function<void(cairo_text_cluster_t*)>> sp_tc(cairo_text_cluster_allocate(tcSize), &cairo_text_cluster_free);
	auto tc_ptr = sp_tc.get();
	for (auto i = 0; i < tcSize; ++i) {
		tc_ptr[i].num_bytes = clusters[i].num_bytes;
		tc_ptr[i].num_glyphs = clusters[i].num_glyphs;
	}
	auto ctcf = static_cast<cairo_text_cluster_flags_t>(clusterToGlyphsMapReverse ? CAIRO_TEXT_CLUSTER_FLAG_BACKWARD : 0);
	cairo_show_text_glyphs(_Context.get(), utf8.data(), _Container_size_to_int(utf8), vec.data(), _Container_size_to_int(vec), sp_tc.get(), tcSize, ctcf);
}

font_extents surface::get_font_extents() const {
	font_extents result;
	cairo_font_extents_t cfe{ };
	cairo_font_extents(_Context.get(), &cfe);
	result.ascent = cfe.ascent;
	result.descent = cfe.descent;
	result.height = cfe.height;
	result.max_x_advance = cfe.max_x_advance;
	result.max_y_advance = cfe.max_y_advance;
	return result;
}

text_extents surface::get_text_extents(const string& utf8) const {
	text_extents result;
	cairo_text_extents_t cte{ };
	cairo_text_extents(_Context.get(), utf8.c_str(), &cte);
	result.height = cte.height;
	result.width = cte.width;
	result.x_advance = cte.x_advance;
	result.x_bearing = cte.x_bearing;
	result.y_advance = cte.y_advance;
	result.y_bearing = cte.y_bearing;
	return result;
}

text_extents surface::get_glyph_extents(const vector<glyph>& glyphs) const {
	vector<cairo_glyph_t> vec;
	for (const auto& glyph : glyphs) {
		vec.push_back({ glyph.index, glyph.x, glyph.y });
	}
	text_extents result;
	cairo_text_extents_t cfe{ };
	cairo_glyph_extents(_Context.get(), vec.data(), _Container_size_to_int(vec), &cfe);
	result.height = cfe.height;
	result.width = cfe.width;
	result.x_advance = cfe.x_advance;
	result.x_bearing = cfe.x_bearing;
	result.y_advance = cfe.y_advance;
	result.y_bearing = cfe.y_bearing;
	return result;
}
