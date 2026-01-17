#!/usr/bin/env python3
"""
Plot spectral analysis results from TUV-x radiators.

Usage:
    # Generate CSV data first by running the test
    ./build/test/unit/test_spectral_analysis

    # Create plots
    python scripts/plot_spectral_analysis.py

    # Specify output directory
    python scripts/plot_spectral_analysis.py -o plots/
"""

import argparse
import os
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns

# Set style - seaborn with enhanced aesthetics (matching plot_solver_benchmarks.py)
sns.set_theme(style="whitegrid", context="talk", palette="deep")
sns.set_style("whitegrid", {
    'grid.linestyle': '--',
    'grid.alpha': 0.6,
    'axes.edgecolor': '0.2',
    'axes.linewidth': 1.2,
})

# Font and figure settings
plt.rcParams.update({
    'figure.dpi': 300,
    'savefig.dpi': 300,
    'text.usetex': False,
    'mathtext.fontset': 'dejavusans',
    'font.family': 'sans-serif',
    'font.sans-serif': ['DejaVu Sans', 'Helvetica', 'Arial'],
    'font.size': 14,
    'axes.titlesize': 16,
    'axes.labelsize': 14,
    'xtick.labelsize': 12,
    'ytick.labelsize': 12,
    'legend.fontsize': 12,
    'figure.titlesize': 18,
    'axes.titleweight': 'bold',
    'axes.labelweight': 'medium',
})

# Default data directory
DATA_DIR = os.path.join(os.path.dirname(__file__), '..', 'plots')


def save_figure(fig, output_dir, name):
    """Save figure in both PNG and PDF formats."""
    png_path = os.path.join(output_dir, f'{name}.png')
    pdf_path = os.path.join(output_dir, f'{name}.pdf')
    fig.savefig(png_path, dpi=300, bbox_inches='tight')
    fig.savefig(pdf_path, bbox_inches='tight')
    print(f"Saved: {png_path}, {pdf_path}")


def plot_cross_section_spectra(data_dir, output_dir):
    """Plot cross-section spectra for Rayleigh, O3, and O2."""
    csv_path = os.path.join(data_dir, 'cross_section_spectra.csv')
    if not os.path.exists(csv_path):
        print(f"Warning: {csv_path} not found, skipping cross-section plot")
        return

    df = pd.read_csv(csv_path)
    colors = sns.color_palette("deep")

    fig, ax = plt.subplots(figsize=(12, 7))

    # Plot each cross-section on log scale
    ax.semilogy(df['wavelength_nm'], df['rayleigh_cm2'], '-', linewidth=2.5,
                color=colors[0], label=r'Rayleigh ($\lambda^{-4}$)')
    ax.semilogy(df['wavelength_nm'], df['o3_cm2'], '-', linewidth=2.5,
                color=colors[1], label=r'O$_3$ (Hartley/Huggins)')
    ax.semilogy(df['wavelength_nm'], df['o2_cm2'], '-', linewidth=2.5,
                color=colors[2], label=r'O$_2$ (Schumann-Runge)')

    # Shade UV regions
    ax.axvspan(200, 280, alpha=0.1, color='purple', label='UV-C')
    ax.axvspan(280, 315, alpha=0.1, color='blue', label='UV-B')
    ax.axvspan(315, 400, alpha=0.1, color='cyan', label='UV-A')

    ax.set_xlabel(r'Wavelength (nm)')
    ax.set_ylabel(r'Cross-section (cm$^2$/molecule)')
    ax.set_title(r'Absorption Cross-Sections')
    ax.set_xlim(150, 700)
    ax.set_ylim(1e-40, 1e-16)
    ax.legend(framealpha=0.9, loc='upper right', ncol=2)

    # Add annotations for key features
    ax.annotate('Hartley band\n(peak ~255 nm)', xy=(255, 1e-17), xytext=(320, 1e-16),
                fontsize=10, ha='center',
                arrowprops=dict(arrowstyle='->', color='gray', lw=1.5))
    ax.annotate('Schumann-Runge\nbands', xy=(180, 5e-18), xytext=(220, 1e-16),
                fontsize=10, ha='center',
                arrowprops=dict(arrowstyle='->', color='gray', lw=1.5))

    plt.tight_layout()
    save_figure(fig, output_dir, 'cross_section_spectra')
    plt.close()


def plot_optical_depth_by_radiator(data_dir, output_dir):
    """Plot optical depth contributions from each radiator type."""
    csv_path = os.path.join(data_dir, 'optical_depth_by_radiator.csv')
    if not os.path.exists(csv_path):
        print(f"Warning: {csv_path} not found, skipping optical depth plot")
        return

    df = pd.read_csv(csv_path)
    colors = sns.color_palette("deep")

    fig, axes = plt.subplots(1, 2, figsize=(14, 6))

    # Left: Stacked area plot
    ax1 = axes[0]
    ax1.fill_between(df['wavelength_nm'], 0, df['o3_tau'],
                     alpha=0.7, color=colors[1], label=r'O$_3$')
    ax1.fill_between(df['wavelength_nm'], df['o3_tau'], df['o3_tau'] + df['rayleigh_tau'],
                     alpha=0.7, color=colors[0], label='Rayleigh')
    ax1.fill_between(df['wavelength_nm'], df['o3_tau'] + df['rayleigh_tau'],
                     df['o3_tau'] + df['rayleigh_tau'] + df['aerosol_tau'],
                     alpha=0.7, color=colors[3], label='Aerosol')

    ax1.set_xlabel(r'Wavelength (nm)')
    ax1.set_ylabel(r'Column Optical Depth $\tau$')
    ax1.set_title(r'Optical Depth by Radiator Type')
    ax1.set_xlim(200, 700)
    ax1.legend(framealpha=0.9, loc='upper right')

    # Shade UV-B region
    ymax = ax1.get_ylim()[1]
    ax1.axvspan(280, 315, alpha=0.15, color='purple', zorder=0)
    ax1.text(297, ymax * 0.95, 'UV-B', ha='center', va='top', fontsize=11,
             color='purple', fontweight='medium')

    # Right: Log scale line plot
    ax2 = axes[1]
    ax2.semilogy(df['wavelength_nm'], df['o3_tau'], '-', linewidth=2.5,
                 color=colors[1], label=r'O$_3$')
    ax2.semilogy(df['wavelength_nm'], df['rayleigh_tau'], '-', linewidth=2.5,
                 color=colors[0], label='Rayleigh')
    ax2.semilogy(df['wavelength_nm'], df['aerosol_tau'], '-', linewidth=2.5,
                 color=colors[3], label='Aerosol')
    ax2.semilogy(df['wavelength_nm'], df['total_tau'], '--', linewidth=2,
                 color='black', alpha=0.7, label='Total')

    ax2.set_xlabel(r'Wavelength (nm)')
    ax2.set_ylabel(r'Column Optical Depth $\tau$')
    ax2.set_title(r'Optical Depth (Log Scale)')
    ax2.set_xlim(200, 700)
    ax2.legend(framealpha=0.9, loc='upper right')

    # Add horizontal reference lines
    ax2.axhline(y=1.0, color='gray', linestyle=':', linewidth=1.5, alpha=0.7)
    ax2.text(680, 1.1, r'$\tau = 1$', fontsize=10, color='gray', ha='right')

    plt.tight_layout(pad=2.0)
    save_figure(fig, output_dir, 'optical_depth_by_radiator')
    plt.close()


def plot_altitude_flux_spectra(data_dir, output_dir):
    """Plot actinic flux spectra at different altitudes."""
    csv_path = os.path.join(data_dir, 'altitude_flux_spectra.csv')
    if not os.path.exists(csv_path):
        print(f"Warning: {csv_path} not found, skipping altitude flux plot")
        return

    df = pd.read_csv(csv_path)

    # Get altitude column names (all columns except wavelength_nm)
    alt_cols = [col for col in df.columns if col != 'wavelength_nm']

    # Create colormap from high to low altitude
    cmap = plt.cm.viridis
    colors = [cmap(i / (len(alt_cols) - 1)) for i in range(len(alt_cols))]

    fig, axes = plt.subplots(1, 2, figsize=(14, 6))

    # Left: Absolute flux spectra
    ax1 = axes[0]
    for i, col in enumerate(alt_cols):
        # Extract altitude from column name (e.g., "flux_78.5km" -> "78.5 km")
        alt_label = col.replace('flux_', '').replace('km', ' km')
        ax1.semilogy(df['wavelength_nm'], df[col], '-', linewidth=2,
                     color=colors[i], label=alt_label)

    ax1.set_xlabel(r'Wavelength (nm)')
    ax1.set_ylabel(r'Actinic Flux (photons/cm$^2$/s/nm)')
    ax1.set_title(r'Actinic Flux vs Altitude (SZA = 30°)')
    ax1.set_xlim(280, 700)
    ax1.legend(framealpha=0.9, loc='lower right', title='Altitude')

    # Shade UV-B region
    ax1.axvspan(280, 315, alpha=0.15, color='purple', zorder=0)

    # Right: Normalized flux (relative to TOA)
    ax2 = axes[1]
    toa_col = alt_cols[0]  # First column is TOA (highest altitude)
    for i, col in enumerate(alt_cols):
        alt_label = col.replace('flux_', '').replace('km', ' km')
        # Avoid division by zero
        normalized = np.where(df[toa_col] > 0, df[col] / df[toa_col], 0)
        ax2.plot(df['wavelength_nm'], normalized, '-', linewidth=2,
                 color=colors[i], label=alt_label)

    ax2.set_xlabel(r'Wavelength (nm)')
    ax2.set_ylabel(r'Flux / TOA Flux')
    ax2.set_title(r'Relative Attenuation vs Altitude')
    ax2.set_xlim(280, 700)
    ax2.set_ylim(0, 1.15)
    ax2.legend(framealpha=0.9, loc='lower right', title='Altitude')

    # Shade UV-B region
    ax2.axvspan(280, 315, alpha=0.15, color='purple', zorder=0)
    ax2.text(297, 1.1, 'UV-B', ha='center', va='top', fontsize=11,
             color='purple', fontweight='medium')

    # Add horizontal reference line
    ax2.axhline(y=1.0, color='gray', linestyle=':', linewidth=1.5, alpha=0.7)

    plt.tight_layout(pad=2.0)
    save_figure(fig, output_dir, 'altitude_flux_spectra')
    plt.close()


def plot_transmittance_spectrum(data_dir, output_dir):
    """Plot atmospheric transmittance spectrum."""
    csv_path = os.path.join(data_dir, 'transmittance_spectrum.csv')
    if not os.path.exists(csv_path):
        print(f"Warning: {csv_path} not found, skipping transmittance plot")
        return

    df = pd.read_csv(csv_path)
    colors = sns.color_palette("deep")

    fig, axes = plt.subplots(1, 2, figsize=(14, 6))

    # Left: Transmittance
    ax1 = axes[0]
    ax1.plot(df['wavelength_nm'], df['transmittance'], '-', linewidth=2.5,
             color=colors[2])
    ax1.fill_between(df['wavelength_nm'], 0, df['transmittance'],
                     alpha=0.3, color=colors[2])

    ax1.set_xlabel(r'Wavelength (nm)')
    ax1.set_ylabel(r'Transmittance (Surface / TOA)')
    ax1.set_title(r'Atmospheric Transmittance (SZA = 0°, 300 DU O$_3$)')
    ax1.set_xlim(280, 700)
    ax1.set_ylim(0, 1.1)

    # Shade UV regions
    ax1.axvspan(280, 315, alpha=0.15, color='purple', zorder=0)
    ax1.axvspan(315, 400, alpha=0.1, color='blue', zorder=0)
    ax1.text(297, 1.05, 'UV-B', ha='center', fontsize=11, color='purple', fontweight='medium')
    ax1.text(357, 1.05, 'UV-A', ha='center', fontsize=11, color='blue', fontweight='medium')

    # Add reference lines
    ax1.axhline(y=0.5, color='gray', linestyle=':', linewidth=1.5, alpha=0.7)
    ax1.text(680, 0.52, '50%', fontsize=10, color='gray', ha='right')

    # Right: TOA and surface flux comparison
    ax2 = axes[1]
    ax2.semilogy(df['wavelength_nm'], df['toa_flux'], '-', linewidth=2.5,
                 color=colors[0], label='TOA (80 km)')
    ax2.semilogy(df['wavelength_nm'], df['surface_flux'], '-', linewidth=2.5,
                 color=colors[1], label='Surface')

    ax2.set_xlabel(r'Wavelength (nm)')
    ax2.set_ylabel(r'Actinic Flux (photons/cm$^2$/s/nm)')
    ax2.set_title(r'TOA vs Surface Flux')
    ax2.set_xlim(280, 700)
    ax2.legend(framealpha=0.9, loc='lower right')

    # Shade UV-B region
    ax2.axvspan(280, 315, alpha=0.15, color='purple', zorder=0)

    # Add annotation showing attenuation
    ax2.annotate('Strong UV-B\nattenuation\nby O$_3$', xy=(300, 5e12), xytext=(350, 2e12),
                 fontsize=10, ha='center',
                 arrowprops=dict(arrowstyle='->', color='gray', lw=1.5))

    plt.tight_layout(pad=2.0)
    save_figure(fig, output_dir, 'transmittance_spectrum')
    plt.close()


def plot_summary_figure(data_dir, output_dir):
    """Create a 2x2 summary figure with all key results."""
    colors = sns.color_palette("deep")

    fig, axes = plt.subplots(2, 2, figsize=(14, 12))

    # Top-left: Cross-sections
    ax1 = axes[0, 0]
    csv_path = os.path.join(data_dir, 'cross_section_spectra.csv')
    if os.path.exists(csv_path):
        df = pd.read_csv(csv_path)
        ax1.semilogy(df['wavelength_nm'], df['rayleigh_cm2'], '-', linewidth=2,
                     color=colors[0], label='Rayleigh')
        ax1.semilogy(df['wavelength_nm'], df['o3_cm2'], '-', linewidth=2,
                     color=colors[1], label=r'O$_3$')
        ax1.semilogy(df['wavelength_nm'], df['o2_cm2'], '-', linewidth=2,
                     color=colors[2], label=r'O$_2$')
        ax1.set_xlim(150, 700)
        ax1.set_ylim(1e-40, 1e-16)
    ax1.set_xlabel(r'Wavelength (nm)')
    ax1.set_ylabel(r'Cross-section (cm$^2$)')
    ax1.set_title(r'Absorption Cross-Sections')
    ax1.legend(framealpha=0.9, loc='upper right')

    # Top-right: Optical depth
    ax2 = axes[0, 1]
    csv_path = os.path.join(data_dir, 'optical_depth_by_radiator.csv')
    if os.path.exists(csv_path):
        df = pd.read_csv(csv_path)
        ax2.semilogy(df['wavelength_nm'], df['o3_tau'], '-', linewidth=2,
                     color=colors[1], label=r'O$_3$')
        ax2.semilogy(df['wavelength_nm'], df['rayleigh_tau'], '-', linewidth=2,
                     color=colors[0], label='Rayleigh')
        ax2.semilogy(df['wavelength_nm'], df['aerosol_tau'], '-', linewidth=2,
                     color=colors[3], label='Aerosol')
        ax2.set_xlim(200, 700)
    ax2.set_xlabel(r'Wavelength (nm)')
    ax2.set_ylabel(r'Column Optical Depth')
    ax2.set_title(r'Optical Depth by Radiator')
    ax2.legend(framealpha=0.9, loc='upper right')
    ax2.axhline(y=1.0, color='gray', linestyle=':', linewidth=1.5, alpha=0.7)

    # Bottom-left: Altitude flux
    ax3 = axes[1, 0]
    csv_path = os.path.join(data_dir, 'altitude_flux_spectra.csv')
    if os.path.exists(csv_path):
        df = pd.read_csv(csv_path)
        alt_cols = [col for col in df.columns if col != 'wavelength_nm']
        cmap = plt.cm.viridis
        for i, col in enumerate(alt_cols):
            color = cmap(i / (len(alt_cols) - 1))
            alt_label = col.replace('flux_', '').replace('km', '')
            ax3.semilogy(df['wavelength_nm'], df[col], '-', linewidth=1.5,
                         color=color, label=f'{alt_label} km')
        ax3.set_xlim(280, 700)
    ax3.set_xlabel(r'Wavelength (nm)')
    ax3.set_ylabel(r'Actinic Flux')
    ax3.set_title(r'Flux vs Altitude (SZA = 30°)')
    ax3.legend(framealpha=0.9, loc='lower right', fontsize=10)

    # Bottom-right: Transmittance
    ax4 = axes[1, 1]
    csv_path = os.path.join(data_dir, 'transmittance_spectrum.csv')
    if os.path.exists(csv_path):
        df = pd.read_csv(csv_path)
        ax4.plot(df['wavelength_nm'], df['transmittance'], '-', linewidth=2.5,
                 color=colors[2])
        ax4.fill_between(df['wavelength_nm'], 0, df['transmittance'],
                         alpha=0.3, color=colors[2])
        ax4.set_xlim(280, 700)
        ax4.set_ylim(0, 1.1)
        ax4.axvspan(280, 315, alpha=0.15, color='purple', zorder=0)
    ax4.set_xlabel(r'Wavelength (nm)')
    ax4.set_ylabel(r'Transmittance')
    ax4.set_title(r'Atmospheric Transmittance (SZA = 0°)')
    ax4.axhline(y=0.5, color='gray', linestyle=':', linewidth=1.5, alpha=0.7)

    plt.tight_layout(pad=2.5)
    save_figure(fig, output_dir, 'spectral_analysis_summary')
    plt.close()


def main():
    parser = argparse.ArgumentParser(description='Plot TUV-x spectral analysis results')
    parser.add_argument('-d', '--data-dir', default=DATA_DIR,
                        help='Directory containing CSV data files')
    parser.add_argument('-o', '--output-dir', default='.',
                        help='Output directory for plots (default: current directory)')
    args = parser.parse_args()

    # Resolve data directory
    data_dir = os.path.abspath(args.data_dir)
    output_dir = os.path.abspath(args.output_dir)

    print(f"Data directory: {data_dir}")
    print(f"Output directory: {output_dir}")

    # Create output directory if needed
    if not os.path.exists(output_dir):
        os.makedirs(output_dir)

    # List available CSV files
    csv_files = [f for f in os.listdir(data_dir) if f.endswith('.csv')]
    print(f"Found CSV files: {', '.join(csv_files)}")

    # Generate plots
    print("\nGenerating plots...")
    plot_cross_section_spectra(data_dir, output_dir)
    plot_optical_depth_by_radiator(data_dir, output_dir)
    plot_altitude_flux_spectra(data_dir, output_dir)
    plot_transmittance_spectrum(data_dir, output_dir)
    plot_summary_figure(data_dir, output_dir)

    print("\nDone!")


if __name__ == '__main__':
    main()
