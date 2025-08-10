function rebuild_and_save_figs_octave()
  exportDir = fullfile('.output','fig_exports');
  if ~exist(exportDir, 'dir'), error('Folder not found: %s', exportDir); end
  make_errorbar_fig(fullfile(exportDir,'clear_errorbar.mat'),   fullfile(exportDir,'clear_errorbar.fig'));
  make_scatter_fig (fullfile(exportDir,'clear_scatter.mat'),    fullfile(exportDir,'clear_scatter.fig'));
  make_regress_fig (fullfile(exportDir,'clear_regression.mat'), fullfile(exportDir,'clear_regression.fig'));
  make_errorbar_fig(fullfile(exportDir,'wall_errorbar.mat'),    fullfile(exportDir,'wall_errorbar.fig'));
  make_scatter_fig (fullfile(exportDir,'wall_scatter.mat'),     fullfile(exportDir,'wall_scatter.fig'));
  make_regress_fig (fullfile(exportDir,'wall_regression.mat'),  fullfile(exportDir,'wall_regression.fig'));
  make_moving_fig  (fullfile(exportDir,'moving_moving.mat'),    fullfile(exportDir,'moving_moving.fig'));
  disp('All .fig and .png files saved to ./.output/fig_exports');
end

function apply_common_axes_style(ax, S)
  grid(ax,'on'); box(ax,'on');
  if isfield(S,'grid_linestyle')
    try, set(ax,'GridLineStyle',S.grid_linestyle); catch, end
  end
end

function set_fig_size(fig_h, w, h)
  if ~isempty(w) && ~isempty(h), set(fig_h,'Position',[100 100 w h]); end
end

function set_xticks_if_available(ax, ticks_vec)
  if isempty(ticks_vec), return; end
  try, xticks(ax, ticks_vec); catch, try, set(ax,'XTick',ticks_vec); catch, end, end
end

function save_fig_and_png(fig_h, outFig)
  [p,n,~] = fileparts(outFig);
  outPng = fullfile(p, [n '.png']);
  ok = false;
  try, hgsave(fig_h, outFig, '-V6'); ok = true; catch, end
  if ~ok
    try, hgsave(fig_h, outFig); ok = true; catch, end
  end
  if ~ok
    try, savefig(fig_h, fullfile(p,[n '.ofig'])); catch, end
  end
  try, print(fig_h, outPng, '-dpng', '-r300'); catch, warning('Could not save PNG for %s', outFig); end
  close(fig_h);
end

function make_errorbar_fig(matFile, outFig)
  S = load(matFile);
  f = figure('Visible','off'); set_fig_size(f,S.fig_w,S.fig_h);
  ax = axes('Parent',f); hold(ax,'on');
  col = [S.color_err_r, S.color_err_g, S.color_err_b];
  h = errorbar(S.x, S.avg, S.yerr_lower, S.yerr_upper, 'o-');
  if isfield(S,'err_marker_size'), set(h,'MarkerSize',S.err_marker_size); end
  if isfield(S,'err_line_width'),  set(h,'LineWidth', S.err_line_width);  end
  set(h,'Color',col,'MarkerEdgeColor',col,'MarkerFaceColor','none');
  xlabel(S.xlabel); ylabel(S.ylabel); title(S.title);
  legend(S.legend_rssi,'Location','northeast');
  set_xticks_if_available(ax,S.xticks_vec);
  apply_common_axes_style(ax,S);
  hold(ax,'off'); drawnow;
  save_fig_and_png(f,outFig);
end

function make_scatter_fig(matFile, outFig)
  S = load(matFile);
  f = figure('Visible','off'); set_fig_size(f,S.fig_w,S.fig_h);
  ax = axes('Parent',f); hold(ax,'on');
  sc_s_col = [S.sc_s_r, S.sc_s_g, S.sc_s_b];
  if ~isempty(S.sx)
    hs = scatter(S.sx, S.sy, S.sc_s_size, 'o', 'MarkerFaceColor', sc_s_col, 'MarkerEdgeColor', sc_s_col);
    if isfield(S,'sc_s_alpha'), try, set(hs,'MarkerFaceAlpha',S.sc_s_alpha,'MarkerEdgeAlpha',S.sc_s_alpha); catch, end, end
  end
  sc_f_col = [S.sc_f_r, S.sc_f_g, S.sc_f_b];
  if ~isempty(S.fx)
    hf = scatter(S.fx, S.fy, S.sc_f_size, 'x', 'MarkerEdgeColor', sc_f_col);
    if isfield(S,'sc_f_alpha'), try, set(hf,'MarkerEdgeAlpha',S.sc_f_alpha); catch, end, end
  end
  if isfield(S,'dist_vals') && ~isempty(S.dist_vals)
    for i=1:numel(S.dist_vals)
      txt = sprintf('S:%d\nF:%d', S.succ_cnt(i), S.fail_cnt(i));
      y_here = S.avg_at_dist(i) + S.y_off;
      text(ax, S.dist_vals(i), y_here, txt, 'VerticalAlignment','top', 'HorizontalAlignment','left', 'BackgroundColor',[S.text_bg_r,S.text_bg_g,S.text_bg_b], 'Margin',2, 'Clipping','on');
    end
  end
  xlabel(S.xlabel); ylabel(S.ylabel); title(S.title);
  legend({S.legend_success,S.legend_failed},'Location','northeast');
  set_xticks_if_available(ax,S.xticks_vec);
  apply_common_axes_style(ax,S);
  hold(ax,'off'); drawnow;
  save_fig_and_png(f,outFig);
end

function make_regress_fig(matFile, outFig)
  S = load(matFile);
  f = figure('Visible','off'); set_fig_size(f,S.fig_w,S.fig_h);
  ax = axes('Parent',f); hold(ax,'on');
  if isfield(S,'sx') && numel(S.sx) >= 2 && ~isnan(S.slope) && ~isnan(S.intercept)
    rx = linspace(min(S.sx), max(S.sx), 100);
    reg_col = [S.reg_r, S.reg_g, S.reg_b];
    plot(ax, rx, S.slope.*rx + S.intercept, 'LineWidth', S.reg_lw, 'Color', reg_col, 'DisplayName','Regression Line');
  end
  sc_s_col = [S.sc_s_r, S.sc_s_g, S.sc_s_b];
  if ~isempty(S.sx)
    hs = scatter(ax, S.sx, S.sy, S.sc_s_size, 'o', 'MarkerFaceColor', sc_s_col, 'MarkerEdgeColor', sc_s_col, 'DisplayName','Success');
    if isfield(S,'sc_s_alpha'), try, set(hs,'MarkerFaceAlpha',S.sc_s_alpha,'MarkerEdgeAlpha',S.sc_s_alpha); catch, end, end
  end
  xlabel(S.xlabel); ylabel(S.ylabel); title(S.title);
  legend(ax,'Location','northeast');
  set_xticks_if_available(ax,S.xticks_vec);
  apply_common_axes_style(ax,S);
  hold(ax,'off'); drawnow;
  save_fig_and_png(f,outFig);
end

function make_moving_fig(matFile, outFig)
  S = load(matFile);
  f = figure('Visible','off'); set_fig_size(f,S.fig_w,S.fig_h);
  ax = axes('Parent',f); hold(ax,'on');
  mv_col = [S.mv_r, S.mv_g, S.mv_b];
  if isfield(S,'rx') && ~isempty(S.rx)
    plot(ax, S.rx, S.avg, 'o-', 'LineWidth', S.mv_lw, 'Color', mv_col, 'MarkerEdgeColor', mv_col, 'MarkerFaceColor','none', 'DisplayName', S.legend_avg);
  end
  fail_col = [S.fail_r, S.fail_g, S.fail_b];
  if isfield(S,'fx') && ~isempty(S.fx)
    hf = scatter(ax, S.fx, S.fy, S.fail_size, 'x', 'MarkerEdgeColor', fail_col, 'DisplayName', S.legend_failed);
    if isfield(S,'fail_alpha'), try, set(hf,'MarkerEdgeAlpha',S.fail_alpha); catch, end, end
  end
  xlabel(S.xlabel); ylabel(S.ylabel); title(S.title);
  legend(ax,'Location','northeast');
  set_xticks_if_available(ax,S.xticks_vec);
  apply_common_axes_style(ax,S);
  hold(ax,'off'); drawnow;
  save_fig_and_png(f,outFig);
end
