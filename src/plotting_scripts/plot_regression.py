#!/usr/bin/env python3
"""
Regression Plot Generator

This script generates various plots for regression models using data passed from the C++ application.
It supports multiple plot types including scatter plots, time series plots, and feature importance plots.

Usage:
    Called from the C++ application with appropriate arguments.
    
Arguments:
    --data_file: Path to CSV file containing the data
    --output_file: Path to save the plot
    --plot_type: Type of plot to generate (scatter, timeseries, importance)
    --actual_col: Name of the actual values column
    --predicted_col: Name of the predicted values column
    --features: List of feature names and their importance scores (for importance plot)
    --title: Plot title
"""

import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import argparse
import os
import sys
import json

def create_scatter_plot(data, actual_col, predicted_col, title, output_file):
    """Create scatter plot of actual vs predicted values."""
    plt.figure(figsize=(10, 6))
    
    # Get actual and predicted values
    actual = data[actual_col]
    predicted = data[predicted_col]
    
    # Create scatter plot
    plt.scatter(actual, predicted, alpha=0.6, label='Data Points')
    
    # Add perfect prediction line
    min_val = min(actual.min(), predicted.min())
    max_val = max(actual.max(), predicted.max())
    plt.plot([min_val, max_val], [min_val, max_val], 'r--', label='Perfect Prediction')
    
    # Add labels and title
    plt.xlabel('Actual Values')
    plt.ylabel('Predicted Values')
    plt.title(title)
    plt.legend()
    plt.grid(True, linestyle='--', alpha=0.7)
    
    # Save plot
    plt.savefig(output_file, format='png', dpi=150)
    plt.close()

def create_timeseries_plot(data, actual_col, predicted_col, title, output_file):
    """Create time series plot of actual and predicted values."""
    plt.figure(figsize=(12, 6))
    
    # Get actual and predicted values
    actual = data[actual_col]
    predicted = data[predicted_col]
    
    # Create time series plot
    plt.plot(actual.index, actual, label='Actual', alpha=0.7)
    plt.plot(predicted.index, predicted, label='Predicted', alpha=0.7)
    
    # Add labels and title
    plt.xlabel('Time Index')
    plt.ylabel('Values')
    plt.title(title)
    plt.legend()
    plt.grid(True, linestyle='--', alpha=0.7)
    
    # Save plot
    plt.savefig(output_file, format='png', dpi=150)
    plt.close()

def create_importance_plot(features_json, title, output_file):
    """Create bar plot of feature importance scores."""
    plt.figure(figsize=(10, 6))
    
    # Parse feature importance data
    features = json.loads(features_json)
    names = list(features.keys())
    scores = list(features.values())
    
    # Sort by importance
    sorted_idx = np.argsort(scores)
    pos = np.arange(len(sorted_idx)) + .5
    
    # Create horizontal bar plot
    plt.barh(pos, np.array(scores)[sorted_idx], align='center')
    plt.yticks(pos, np.array(names)[sorted_idx])
    
    # Add labels and title
    plt.xlabel('Relative Importance')
    plt.title(title)
    plt.grid(True, linestyle='--', alpha=0.7)
    
    # Save plot
    plt.savefig(output_file, format='png', dpi=150)
    plt.close()

def main():
    parser = argparse.ArgumentParser(description='Generate regression model plots')
    parser.add_argument('--data_file', help='Path to CSV file containing the data')
    parser.add_argument('--output_file', required=True, help='Path to save the plot')
    parser.add_argument('--plot_type', required=True, choices=['scatter', 'timeseries', 'importance'],
                      help='Type of plot to generate')
    parser.add_argument('--actual_col', help='Name of the actual values column')
    parser.add_argument('--predicted_col', help='Name of the predicted values column')
    parser.add_argument('--features', help='JSON string of feature names and importance scores')
    parser.add_argument('--title', default='Regression Results', help='Plot title')
    
    args = parser.parse_args()
    
    try:
        # Create output directory if it doesn't exist
        os.makedirs(os.path.dirname(args.output_file), exist_ok=True)
        
        if args.plot_type in ['scatter', 'timeseries']:
            if not args.data_file or not args.actual_col or not args.predicted_col:
                raise ValueError("Data file and column names are required for scatter and timeseries plots")
            
            # Load data
            data = pd.read_csv(args.data_file)
            
            if args.plot_type == 'scatter':
                create_scatter_plot(data, args.actual_col, args.predicted_col, args.title, args.output_file)
            else:  # timeseries
                create_timeseries_plot(data, args.actual_col, args.predicted_col, args.title, args.output_file)
        
        elif args.plot_type == 'importance':
            if not args.features:
                raise ValueError("Feature importance data is required for importance plot")
            create_importance_plot(args.features, args.title, args.output_file)
        
        print(f"Plot saved as: {os.path.abspath(args.output_file)}")
        sys.exit(0)
        
    except Exception as e:
        print(f"Error creating plot: {str(e)}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()